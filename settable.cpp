#define S_FUNCTION_NAME   settable //Cambiar este nombre por el nombre de su s-Function
#define S_FUNCTION_LEVEL 2
#define NUM_PARAMS 1 //Numero de parametros esperados
#include "simstruc.h"
#include "isharedmemory.h"

using namespace ilib;
using namespace std;

//custom functions y declare
ISharedMemoryContainer allTables;
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

size_t getInputSizeInBytes(SimStruct *S,int_T  port){
    auto width = ssGetInputPortWidth(S,port);
    size_t memsize=0;
    switch(ssGetInputPortDataType(S,port)){
        case SS_DOUBLE:
            memsize= width*8;
            break;
        case SS_SINGLE:
            memsize= width*4;
            break;
        case SS_INT8:
            memsize= width;
            break;
        case SS_UINT8:
            memsize= width;
            break;
        case SS_INT16:
            memsize= width*2;
            break;
        case SS_UINT16:
            memsize= width*2;
            break;
        case SS_INT32:
            memsize= width*4;
            break;
        case SS_UINT32:
            memsize= width*4;
            break;
        case SS_BOOLEAN: //¿como char?
            memsize= width;
            break;
    }
    return memsize;
}

#define NUM_PARAMS 1
enum PARAMETERS{P_TABLE_NAME};

//****************Mex functions***********//////
#define MDL_CHECK_PARAMETERS
#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
static void mdlCheckParameters(SimStruct *S)
{
    size_t nameLength = (mxGetM(ssGetSFcnParam(S,P_TABLE_NAME)) * mxGetN(ssGetSFcnParam(S,P_TABLE_NAME))) + 1;
    if(nameLength<2)
    {
        ssSetErrorStatus(S,"Table Name Field can not be empty.\n");
        printf("Register name length= %i",nameLength);
        return;
    }
}
#endif /* MDL_CHECK_PARAMETERS */

static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, NUM_PARAMS);
#if defined(MATLAB_MEX_FILE)
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) return;
    mdlCheckParameters(S);
    if (ssGetErrorStatus(S) != NULL) return;
#endif
    /* Set number of input and output ports */
    if (!ssSetNumInputPorts( S,1)) return;

    if (!ssSetNumOutputPorts(S,0)) return;

    ssSetInputPortWidth (S, 0, DYNAMICALLY_SIZED);
    ssSetInputPortDataType(S,0,DYNAMICALLY_TYPED); 
    ssSetInputPortDirectFeedThrough(S, 0, TRUE);
//no se sabe a priori el tipo de dato
    //ssSetInputPortDimensionInfo( S, 0, DYNAMIC_DIMENSION); //no se sabe a priori eltamaño
    ssSetInputPortRequiredContiguous(S, 0, 1);


    ssSetNumSampleTimes(S, 1);
    
    /* specify the sim state compliance to be same as a built-in block */
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);
    
    ssSetOptions(S,
            SS_OPTION_WORKS_WITH_CODE_REUSE |
            SS_OPTION_EXCEPTION_FREE_CODE);
} 

static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetModelReferenceSampleTimeDefaultInheritance(S);
} 

#if defined(MATLAB_MEX_FILE)
#define MDL_SET_INPUT_PORT_DIMENSION_INFO
static void mdlSetInputPortDimensionInfo(SimStruct *S,int_T port,const DimsInfo_T *dimsInfo)
{
    if(!ssSetInputPortDimensionInfo(S, port, dimsInfo)) return;

} /* end mdlSetInputPortDimensionInfo */

# define MDL_SET_DEFAULT_PORT_DIMENSION_INFO
static void mdlSetDefaultPortDimensionInfo(SimStruct *S)
{
    int_T inWidth = ssGetInputPortWidth(S, 0);
    /* Input port dimension must be unknown. Set it to scalar. */
    if(inWidth == DYNAMICALLY_SIZED){
        DECL_AND_INIT_DIMSINFO(di);
        int_T dims[1];
        di.numDims = 1;
        dims[0] = 128;
        di.dims = dims;
        di.width = dims[0];
        ssSetInputPortDimensionInfo(S,  0, &di);
    }
    auto dims = ssGetUserData(S);
    free(dims);
} /* end mdlSetDefaultPortDimensionInfo */
#endif

#define MDL_SET_INPUT_PORT_DATA_TYPE
void mdlSetInputPortDataType(SimStruct *S, int_T port,DTypeId id){
    if(!ssSetInputPortDataType(S, port, id)) return;
}

#define MDL_SET_DEFAULT_PORT_DATA_TYPES
void mdlSetDefaultPortDataTypes(SimStruct *S){
    if (ssGetInputPortDataType(S, 0) == DYNAMICALLY_TYPED) {
        ssSetInputPortDataType(S, 0, SS_UINT8 );
    }
}

#define MDL_START  /* Change to #undef to remove function */
static void mdlStart(SimStruct *S)
{
    
    char *tableName = mxArrayToString(ssGetSFcnParam(S,P_TABLE_NAME));
    auto imem=allTables.getFromName(tableName);
    if(imem){
        allTables.removeFromName(tableName);
    }
    //Crear memoria compartida y agregarla al contenedor.
    imem=new ISharedMemory(tableName);
    imem->create(getInputSizeInBytes(S,0)+sizeof(TableHead));
    if(imem){
        //Rellenar la cabecera
        allTables.insert(imem);
    }
    else{
        ssSetErrorStatus(S,"Table Name Field can not be empty.\n");
        return;
    }
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
    //mexPrintf("mdlOutputs ");
    //1 determino el tamaño y tipo de dato.
    auto *ptr_data=ssGetInputPortSignal(S,0);   //buffer de datos para numerico.
    auto simDataSize=ssGetInputPortWidth(S,0);      //cantidad de elementos en la tabla
    auto simDataSize_bytes=getInputSizeInBytes(S,0); //tamaño de los datos en bytes
     
    auto dataType=TABLE_NULL;
    auto simulink_type=ssGetInputPortDataType(S,0);
    if(simulink_type==SS_DOUBLE)
    {
        dataType=TABLE_NUMBER;
    }
    else //SE PODRIA PERSONALIZAR PARA DISTINTOS TIPOS, pero los plugin de vrep por ahora solo permiten diferenciar entre string y arreglo numerico.
    {
        dataType=TABLE_STRING;
    }
    
    //2 determino el nombre de la memoria, su tamaño y la coloco en el contenedor si no existe.
    auto tableName= mxArrayToString(ssGetSFcnParam(S,P_TABLE_NAME));
    auto imem=allTables.getFromName(tableName);
    if(!imem){ //la memoria compartida no esta registrada. Se crea una nueva memoria.
        imem= new ISharedMemory(tableName);
        auto mem_size=simDataSize_bytes+sizeof(TableHead);
        imem->create(mem_size);
        if(!imem){
            ssSetErrorStatus(S,"Failed to create or attach to existing table. Try changing the name of the table.");
            return;
        }
        allTables.insert(imem);
    }
    
    //Actualizar header
    TableHead header;
    //get actual Header
    imem->getData((char*)nullptr,0,&header,1);
    //update header
    header.type=dataType;
    header.prc_cnt++;
        //4 realizo la escritura de datos y header. diferenciando la fuente segun el tipo de dato

    if(dataType==TABLE_NUMBER){
        header.rows=simDataSize;
        imem->setData(reinterpret_cast<const double*>(ptr_data),simDataSize,&header,1);
    }else{
        header.rows=simDataSize_bytes;
        imem->setData(reinterpret_cast<const uint8_t*>(ptr_data),simDataSize_bytes,&header,1);      
    }   
    
     //mexPrintf("Port width =%i ",header.rows);
    double*ptr=(double*)((char*)imem->data())+sizeof(TableHead);
    for(auto i=0;i<simDataSize;i++){
        mexPrintf("%f ",ptr[i]);
    }
}

static void mdlTerminate(SimStruct *S)
{
    
    char *tableName = mxArrayToString(ssGetSFcnParam(S,P_TABLE_NAME));
    auto imem=allTables.getFromName(tableName);
    if(imem)
        mexPrintf("mdlTerminate Mem=%s; Size=%i",imem->key(),imem->size());
    allTables.removeFromName(tableName);
    auto dims = ssGetUserData(S);
    free(dims);
} /* end mdlTerminate */

#ifdef	MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif