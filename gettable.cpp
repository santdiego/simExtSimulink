#define S_FUNCTION_NAME  gettable
#define S_FUNCTION_LEVEL 2

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


#define NUM_PARAMS 1
enum PARAMETERS{P_TABLE_NAME};

ISharedMemory* searchAttach(SimStruct *S){
    auto tableName= mxArrayToString(ssGetSFcnParam(S,P_TABLE_NAME));
    auto imem=allTables.getFromName(tableName);
    if(imem==nullptr){
        imem= new ISharedMemory(tableName);
        if(imem->attach())
            allTables.insert(imem);
        else
            return nullptr;
    }
    return imem;
}

#ifdef MATLAB_MEX_FILE
#define MDL_CHECK_PARAMETERS
/* Function: mdlCheckParameters =============================================
 * Abstract:
 *    Verify parameter settings.
 */
static void mdlCheckParameters(SimStruct *S)
{
    size_t nameLength = (mxGetM(ssGetSFcnParam(S,P_TABLE_NAME)) * mxGetN(ssGetSFcnParam(S,P_TABLE_NAME))) + 1;
    if(nameLength<2)
    {
        ssSetErrorStatus(S,"Table Name Field can not be empty.\n");
        printf("Register name length= %i",nameLength);
        return;
    }
} /* end mdlCheckParameters */
#endif

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *   Initialize the sizes array
 */
static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, NUM_PARAMS);
    
#if defined(MATLAB_MEX_FILE)
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) return;
    mdlCheckParameters(S);
    if (ssGetErrorStatus(S) != NULL) return;
#endif
    
    /* Set number of input and output ports */
    if (!ssSetNumInputPorts( S,0)) return;
    if (!ssSetNumOutputPorts(S,1)) return;
    
    auto imem=searchAttach(S);
    if(imem){
        TableHead header;
        imem->getData((char*)nullptr,0,&header,1);
        ssSetOutputPortWidth(S,0,header.rows);
        if(header.type==TABLE_NUMBER){
            if(!ssSetOutputPortDataType(S, 0, SS_DOUBLE)) return;
        }
        else{
            if(!ssSetOutputPortDataType(S, 0, SS_UINT8 )) return;
        }
    }
    else{
        if(!ssSetOutputPortDimensionInfo(S, 0, DYNAMIC_DIMENSION)) return;
        if(!ssSetOutputPortDataType(S, 0, DYNAMICALLY_TYPED)) return;
        
        ssWarning(S,"Memory could not be attached. Verify that the server is active. The output will be dimensioned by means of propagation.");
    }
    
    
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


# define MDL_SET_OUTPUT_PORT_DIMENSION_INFO
static void mdlSetOutputPortDimensionInfo(SimStruct *S,int_T port,const DimsInfo_T *dimsInfo)
{ 
    if(!ssSetOutputPortDimensionInfo(S, port, dimsInfo)) return;
}

# define MDL_SET_DEFAULT_PORT_DIMENSION_INFO
static void mdlSetDefaultPortDimensionInfo(SimStruct *S)
{
    ssWarning(S,"The size of the output vector could not be determined by the server or by propagation. By default, a vector of 128 elements will be used.");
    
    int_T outWidth = ssGetOutputPortWidth(S, 0);
    /* Input port dimension must be unknown. Set it to scalar. */
    if(outWidth == DYNAMICALLY_SIZED){
        DECL_AND_INIT_DIMSINFO(di);
        int_T dims[1];
        di.numDims = 1;
        dims[0] = 128;
        di.dims = dims;
        di.width = dims[0];
        ssSetOutputPortDimensionInfo(S,  0, &di);
    }
} /* end mdlSetDefaultPortDimensionInfo */

#define MDL_SET_OUTPUT_PORT_DATA_TYPE
void mdlSetOutputPortDataType(SimStruct *S, int_T port,DTypeId id){
    if(!ssSetOutputPortDataType(S, port, id)) return;
}

#define MDL_SET_DEFAULT_PORT_DATA_TYPES
void mdlSetDefaultPortDataTypes(SimStruct *S){
    
    if (ssGetOutputPortDataType(S, 0) == DYNAMICALLY_TYPED) {
        ssSetOutputPortDataType(S, 0, SS_UINT8 );
    }
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
    //mexPrintf("mdlOutputs ");
    auto imem =searchAttach(S);
    if(!imem){
        ssWarning(S,"Server not found");
        return;
    }
    TableHead header;
    imem->getData((char*)nullptr,0,&header,1);
    auto port_size=ssGetOutputPortDimensions(S,0)[0];
    if(ssGetOutputPortDataType(S, 0)==SS_DOUBLE)
    {
        //mexPrintf("SS_DOUBLE");
        double *ptr_data=(double*)ssGetOutputPortSignal(S,0);
        imem->getData(ptr_data,port_size,(TableHead*)nullptr,1);
//         double*ptr=(double*)((char*)imem->data()+sizeof(TableHead));
//         for(auto i=0;i<port_size;i++){
//             mexPrintf("%f ",ptr[i]);
    }
    
    else
    {
        //mexPrintf("UINT8_T!");
        uint8_t *ptr_data=(uint8_t*)ssGetOutputPortSignal(S,0);
        imem->getData(ptr_data,port_size,(TableHead*)nullptr,1);
        if(header.type==TABLE_NUMBER)
            imem->getData(ptr_data,port_size*sizeof(double),(TableHead*)nullptr,1);
        else
            imem->getData(ptr_data,port_size,(TableHead*)nullptr,1);
    }
    
    
} /* end mdlOutputs */

static void mdlTerminate(SimStruct *S)
{
    char *tableName = mxArrayToString(ssGetSFcnParam(S,P_TABLE_NAME));
    auto imem=allTables.getFromName(tableName);
    if(imem)
        mexPrintf("mdlTerminate Mem=%s; Size=%i",imem->key(),imem->size());
    allTables.removeFromName(tableName);
    /* Deallocate intdimensions stored in user data*/
    auto dims = ssGetUserData(S);
    free(dims);
} /* end mdlTerminate */

#ifdef	MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif