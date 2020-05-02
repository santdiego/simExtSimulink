/**********************************************************************************************
CoppeliaSim Simulink Communication plugin.
Version beta 1.2
Author : Dr. Ing. Diego Daniel Santiago.
diego.daniel.santiago@gmail.com
dsantiago@inaut.unsj.edu.ar
Facultad de Ingeniería
Universidad Nacional de San Juan
San Juan - Argentina

INAUT - Instituto de Automática
http://www.inaut.unsj.edu.ar/
CONICET - Consejo Nacional de Investigaciones Científicas y Técnicas.
http://www.conicet.gov.ar/
************************************************************************************************/


#include "simExtSimulink.h"
#include "stackArray.h"
#include "stackString.h"
#include "simLib.h"
#include <iostream>
#include "isharedmemory.h"
#include <vector>

using namespace ilib;
using namespace std;


#ifdef _WIN32
#ifdef QT_COMPIL
#include <direct.h>
#else
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif
#endif

#if defined(__linux) || defined(__APPLE__)
#include <unistd.h>
#include <string.h>
#define _stricmp(x,y) strcasecmp(x,y)
#endif

#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)    CONCAT(x,y,z)

#define PLUGIN_VERSION 5 // 2 since version 3.2.1, 3 since V3.3.1, 4 since V3.4.0, 5 since V3.4.1

LIBRARY simLib; // the CoppelisSim library that we will dynamically load and bind

// --------------------------------------------------------------------------------------
// simExtSkeleton_getData: an example of custom Lua command
// --------------------------------------------------------------------------------------
//#define LUA_GETDATA_COMMAND "simSkeleton.getData" // the name of the new Lua command

#define LUA_GETDATA_COMMAND "simulinkTable.getData"
#define LUA_SETDATA_COMMAND "simulinkTable.setData"
enum TableType{
    TABLE_NULL=0,
    TABLE_NUMBER,
    TABLE_BOOL,
    TABLE_STRING,
    TABLE_ARRAY,
    TABLE_MAP
};

struct TableHead{
    TableType type=TABLE_NULL; //table data type
    size_t rows=0;             //number of elements inside table
    size_t prc_cnt=0;          //number of proccess attached to the table.
};

//Datos en memoria: [cantidad de tipos de datos "2*n"][tipo dato 1][tamaño dato 1]...[tipo data n][tamaño dato n][datos en bytes]

ISharedMemoryContainer allTables;

void legend(){
    simSetLastError((string("warning@")+string(LUA_SETDATA_COMMAND)).c_str(),"\n CoppeliaSim Simulink Communication plugin\n"
                                                                             "Version beta 1.2\n"
                                                                             "Author : Dr. Ing. Diego Daniel Santiago.\n"
                                                                             "diego.daniel.santiago@gmail.com\n"
                                                                             "dsantiago@inaut.unsj.edu.ar\n"
                                                                             "Facultad de Ingeniería\n"
                                                                             "Universidad Nacional de San Juan\n"
                                                                             "San Juan - Argentina\n"
                                                                             "INAUT - Instituto de Automática\n"
                                                                             "http://www.inaut.unsj.edu.ar/\n"
                                                                             "CONICET - Consejo Nacional de Investigaciones Científicas y Técnicas.\n"
                                                                             "http://www.conicet.gov.ar/\n");


}

void LUA_SETDATA_CALLBACK(SScriptCallBack* p){

    int stack=p->stackID;
    CStackArray inArguments;
    inArguments.buildFromStack(stack);

    //0 verifico que los argumentos del usuario sean correctos
    if((inArguments.getSize()<2) || !(inArguments.isString(0)) ){
        simSetLastError(LUA_SETDATA_COMMAND,"Not enough arguments or wrong arguments.");
        return;
    }

    //1 determino el tamaño y tipo de dato.
    size_t simDataSize=0;
    TableType dataType=TABLE_NULL;
    string string_data; //buffer de datos para string
    const double *numeric_data;   //buffer de datos para numerico.

    if(inArguments.isString(1))//primera opcion es que sea un buffer de chars
    {
        string_data=inArguments.getString(1);
        simDataSize=string_data.size();
        dataType=TABLE_STRING;
    }
    else if(inArguments.isArray(1)){ //segunda opcion que sea un arreglo
        auto numeric_table=inArguments.getArray(1);
        if(numeric_table->isNumberArray()){ // el arreglo es numerico
            numeric_data=numeric_table->getDoublePointer();
            dataType=TABLE_NUMBER;
            simDataSize=numeric_table->getSize();
        }
        else{  //es otro tipo de arreglo, por ahora no soportado
            simSetLastError(LUA_SETDATA_COMMAND,"Currently, only numerical arrays are supported.");
            return;
        }
    }
    //2 determino el nombre de la memoria, su tamaño y la coloco en el contenedor si no existe.
    auto tableName= inArguments.getString(0);
    auto imem=allTables.getFromName(tableName.c_str());
    if(imem==nullptr){ //la memoria compartida no esta registrada. Se crea una nueva memoria.
        imem= new ISharedMemory(tableName.c_str());
        size_t mem_size=0;
        switch(dataType){
        case TABLE_NUMBER:
            mem_size=sizeof (double)*simDataSize+sizeof(TableHead);
            break;
        case TABLE_STRING:
            mem_size=sizeof (char)*simDataSize+sizeof(TableHead);
            break;
        default:
            simSetLastError(LUA_SETDATA_COMMAND,"Input type not supported.");
            ;
        }
        imem->create(mem_size);
        if(imem){
            //Actualizar header
            TableHead header;
            //get actual Header
            imem->getData((char*)nullptr,0,&header,1);
            //update header
            header.type=dataType;
            header.rows=simDataSize;
            header.prc_cnt++;
            //set Header
            imem->setData((char*)nullptr,0,&header,1);
            //agregar al contenedor
            allTables.insert(imem);
        }
        else{
            simSetLastError(LUA_SETDATA_COMMAND,"Failed to create or attach to existing table. Try changing the name of the table.");
            return;
        }
    }
    //4 realizo la escritura de datos. diferenciando la fuente segun el tipo de dato
    switch(dataType)
    {
    case TABLE_STRING:
        imem->setData(string_data.c_str(),string_data.size(),(TableHead*)nullptr,1);
        break;
    case TABLE_NUMBER:
    {
        imem->setData(numeric_data,simDataSize,(TableHead*)nullptr,1);
    }
        break;
    default:
        simSetLastError(LUA_SETDATA_COMMAND,"Input type not supported.");
        break;
    }
}

void LUA_GETDATA_CALLBACK(SScriptCallBack* p)
{
    int stack=p->stackID;
    CStackArray inArguments;
    inArguments.buildFromStack(stack);
    if((inArguments.getSize()<1) || !(inArguments.isString(0))){
        simSetLastError(LUA_SETDATA_COMMAND,"Not enough arguments or wrong arguments.");
    }

    auto tableName= inArguments.getString(0);

    //1º buscar la tabla, sino adjuntar y agregarla al contenedor
    auto imem=allTables.getFromName(tableName.c_str());
    if(imem==nullptr){ //Table isnt registered
        imem= new ISharedMemory(tableName.c_str());

        if(imem->attach()){
            allTables.insert(imem);
        }
        else{
            simSetLastError((string("warning@")+string(LUA_SETDATA_COMMAND)).c_str(),"Warning: The table was not found in memory. Verify that table is declared and simulation running at remote site . ");
            return;
        }
    }
    //2º Obtengo el tipo de dato de la tabla y el tamaño

    TableHead header;
    imem->getData((char*)nullptr,0,&header,1);
    TableType type=header.type;
    size_t    sim_data_size=header.rows;

    //3º obtengo los datosy los externalizo

    if(type==TABLE_STRING)
    {
        CStackArray outArguments;
        const char *ptr=((const char*)imem->data()+sizeof (TableHead));
        outArguments.pushString(ptr,sim_data_size);
        outArguments.buildOntoStack(stack);
    }
    else if(type==TABLE_NUMBER)
    {
       const double *ptr=reinterpret_cast<const double*>((char*)imem->data()+sizeof (TableHead));
       simPushDoubleTableOntoStack(stack,ptr,sim_data_size);
       //outArguments.setDoubleArray(ptr,sim_data_size);
//       for (size_t i=0;i<sim_data_size;i++) {
//           outArguments.pushDouble(ptr[i]);
//       }
    }
    else
    {
        simSetLastError(LUA_SETDATA_COMMAND,"Data type not supported.");
    }

    //

}
// --------------------------------------------------------------------------------------


// This is the plugin start routine (called just once, just after the plugin was loaded):
SIM_DLLEXPORT unsigned char simStart(void* reservedPointer,int reservedInt)
{
    // Dynamically load and bind CoppelisSim functions:
    // 1. Figure out this plugin's directory:
    char curDirAndFile[1024];
#ifdef _WIN32
#ifdef QT_COMPIL
    _getcwd(curDirAndFile, sizeof(curDirAndFile));
#else
    GetModuleFileName(NULL,curDirAndFile,1023);
    PathRemoveFileSpec(curDirAndFile);
#endif
#else
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    // 2. Append the CoppelisSim library's name:
    std::string temp(currentDirAndPath);
#ifdef _WIN32
    temp+="\\coppeliaSim.dll";
#elif defined (__linux)
    temp+="/libcoppeliaSim.so";
#elif defined (__APPLE__)
    temp+="/libcoppeliaSim.dylib";
#endif /* __linux || __APPLE__ */
    // 3. Load the CoppelisSim library:
    simLib=loadSimLibrary(temp.c_str());
    if (simLib==NULL)
    {
        std::cout << "Error, could not find or correctly load the CoppelisSim library. Cannot start 'ExtSimulink' plugin.\n";
        return(0); // Means error, CoppelisSim will unload this plugin
    }
    if (getSimProcAddresses(simLib)==0)
    {
        std::cout << "Error, could not find all required functions in the CoppelisSim library. Cannot start 'ExtSimulink' plugin.\n";
        unloadSimLibrary(simLib);
        return(0); // Means error, CoppelisSim will unload this plugin
    }

    // Check the version of CoppelisSim:
    int simVer,simRev;
    simGetIntegerParameter(sim_intparam_program_version,&simVer);
    simGetIntegerParameter(sim_intparam_program_revision,&simRev);
    if( (simVer<30400) || ((simVer==30400)&&(simRev<9)) )
    {
        std::cout << "Sorry, your CoppelisSim copy is somewhat old, CoppelisSim 3.4.0 rev9 or higher is required. Cannot start 'ExtSimulink' plugin.\n";
        unloadSimLibrary(simLib);
        return(0); // Means error, CoppelisSim will unload this plugin
    }

    // Implicitely include the script lua/simExtExtSimulink.lua:
    simRegisterScriptVariable("simSkeleton","require('simExtExtSimulink')",0);

    // Register the new function:
    simRegisterScriptCallbackFunction(strConCat(LUA_SETDATA_COMMAND,"@","Simulink"),strConCat("...=",LUA_SETDATA_COMMAND,"(string table_name,array table_content)"),LUA_SETDATA_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_GETDATA_COMMAND,"@","Simulink"),strConCat("table=",LUA_GETDATA_COMMAND,"()"),LUA_GETDATA_CALLBACK);
    return(PLUGIN_VERSION); // initialization went fine, we return the version number of this plugin (can be queried with simGetModuleName)
}

// This is the plugin end routine (called just once, when CoppelisSim is ending, i.e. releasing this plugin):
SIM_DLLEXPORT void simEnd()
{
    // Here you could handle various clean-up tasks

    unloadSimLibrary(simLib); // release the library
}

// This is the plugin messaging routine (i.e. CoppelisSim calls this function very often, with various messages):
SIM_DLLEXPORT void* simMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{ // This is called quite often. Just watch out for messages/events you want to handle
    // Keep following 5 lines at the beginning and unchanged:
    static bool refreshDlgFlag=true;
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode,&errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode,sim_api_errormessage_ignore);
    void* retVal=NULL;

    // Here we can intercept many messages from CoppelisSim (actually callbacks). Only the most important messages are listed here.
    // For a complete list of messages that you can intercept/react with, search for "sim_message_eventcallback"-type constants
    // in the CoppelisSim user manual.

    if (message==sim_message_eventcallback_refreshdialogs)
        refreshDlgFlag=true; // CoppelisSim dialogs were refreshed. Maybe a good idea to refresh this plugin's dialog too

    if (message==sim_message_eventcallback_menuitemselected)
    { // A custom menu bar entry was selected..
        // here you could make a plugin's main dialog visible/invisible
    }

    if (message==sim_message_eventcallback_instancepass)
    {   // This message is sent each time the scene was rendered (well, shortly after) (very often)
        // It is important to always correctly react to events in CoppelisSim. This message is the most convenient way to do so:

        int flags=auxiliaryData[0];
        bool sceneContentChanged=((flags&(1+2+4+8+16+32+64+256))!=0); // object erased, created, model or scene loaded, und/redo called, instance switched, or object scaled since last sim_message_eventcallback_instancepass message
        bool instanceSwitched=((flags&64)!=0);

        if (instanceSwitched)
        {
            // React to an instance switch here!!
        }

        if (sceneContentChanged)
        { // we actualize plugin objects for changes in the scene

            //...

            refreshDlgFlag=true; // always a good idea to trigger a refresh of this plugin's dialog here
        }
    }

    if (message==sim_message_eventcallback_mainscriptabouttobecalled)
    { // The main script is about to be run (only called while a simulation is running (and not paused!))
        
    }

    if (message==sim_message_eventcallback_simulationabouttostart)
    { // Simulation is about to start
        legend();
    }

    if (message==sim_message_eventcallback_simulationended)
    { // Simulation just ended

        allTables.removeAll();
    }

    if (message==sim_message_eventcallback_moduleopen)
    { // A script called simOpenModule (by default the main script). Is only called during simulation.
        if ( (customData==NULL)||(_stricmp("ExtSimulink",(char*)customData)==0) ) // is the command also meant for this plugin?
        {
            // we arrive here only at the beginning of a simulation
        }
    }

    if (message==sim_message_eventcallback_modulehandle)
    { // A script called simHandleModule (by default the main script). Is only called during simulation.
        if ( (customData==NULL)||(_stricmp("ExtSimulink",(char*)customData)==0) ) // is the command also meant for this plugin?
        {
            // we arrive here only while a simulation is running
        }
    }

    if (message==sim_message_eventcallback_moduleclose)
    { // A script called simCloseModule (by default the main script). Is only called during simulation.
        if ( (customData==NULL)||(_stricmp("ExtSimulink",(char*)customData)==0) ) // is the command also meant for this plugin?
        {
            // we arrive here only at the end of a simulation
        }
    }

    if (message==sim_message_eventcallback_instanceswitch)
    { // We switched to a different scene. Such a switch can only happen while simulation is not running

    }

    if (message==sim_message_eventcallback_broadcast)
    { // Here we have a plugin that is broadcasting data (the broadcaster will also receive this data!)

    }

    if (message==sim_message_eventcallback_scenesave)
    { // The scene is about to be saved. If required do some processing here (e.g. add custom scene data to be serialized with the scene)

    }

    // You can add many more messages to handle here

    if ((message==sim_message_eventcallback_guipass)&&refreshDlgFlag)
    { // handle refresh of the plugin's dialogs
        // ...
        refreshDlgFlag=false;
    }

    // Keep following unchanged:
    simSetIntegerParameter(sim_intparam_error_report_mode,errorModeSaved); // restore previous settings
    return(retVal);
}

