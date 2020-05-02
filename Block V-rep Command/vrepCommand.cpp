/**********************************************************************************************
Author : Ing. Diego Daniel Santiago.
diego.daniel.santiago@gmail.com
dsantiago@inaut.unsj.edu.ar
Facultad de Ingeniera
Universidad Nacional de San Juan
San Juan - Argentina

INAUT - Instituto de Automatica
http://www.inaut.unsj.edu.ar/
CONICET - Consejo Nacional de Investigaciones Científicas y Técnicas.
http://www.conicet.gov.ar/
 *NOTA de version: 
 *08/10/2015 Lanzamiento Version 1
************************************************************************************************/

#define S_FUNCTION_NAME  vrepCommand
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */

#define NON_MATLAB_PARSING 
#define MAX_EXT_API_CONNECTIONS 255

#include "simstruc.h"
#include "extApi.c"
#include "extApiPlatform.c"



/* Error handling
 * --------------
 *
 * You should use the following technique to report errors encountered within
 * an S-function:
 *
 *       ssSetErrorStatus(S,"Error encountered due to ...");
 *       return;
 *
 * Note that the 2nd argument to ssSetErrorStatus must be persistent memory.
 * It cannot be a local variable. For example the following will cause
 * unpredictable errors:
 *
 *      mdlOutputs()
 *      {
 *         char msg[256];         {ILLEGAL: to fix use "static char msg[256];"}
 *         sprintf(msg,"Error due to %s", string);
 *         ssSetErrorStatus(S,msg);
 *         return;
 *      }
 *
 */

/*====================*
 * S-function methods *
 *====================*/

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */

#define NOMBRE          0

static void mdlInitializeSizes(SimStruct *S)
{
    //Parametro 1 : nombre de la escena: 
	//verificar que no se exceda el maximo y que no este vacio
    static size_t nameLength = (mxGetM(ssGetSFcnParam(S,NOMBRE)) * mxGetN(ssGetSFcnParam(S,NOMBRE))) + 1;
    
    if(nameLength<2||nameLength>50)
    { 
        ssSetErrorStatus(S,"El nombre asignado a la memoria no es Valido \n");
        return;
    }
    
    ssSetNumSFcnParams(S, 1);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        /* Return if number of expected != number of actual parameters */
        return;
    }

    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);

//     if (!ssSetNumInputPorts(S, 0)) return;
//     ssSetInputPortWidth(S, 0, 0);
//     ssSetInputPortRequiredContiguous(S, 0, true); /*direct input signal access*/
    /*
     * Set direct feedthrough flag (1=yes, 0=no).
     * A port has direct feedthrough if the input is used in either
     * the mdlOutputs or mdlGetTimeOfNextVarHit functions.
     */
//     ssSetInputPortDirectFeedThrough(S, 0, 1);

//     if (!ssSetNumOutputPorts(S, 0)) return;
//     ssSetOutputPortWidth(S, 0, 1);

    ssSetNumSampleTimes(S, 1);
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 0);

    /* Specify the sim state compliance to be same as a built-in block */
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);

    ssSetOptions(S, 0);
}



/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);

}



#define MDL_INITIALIZE_CONDITIONS   /* Change to #undef to remove function */
#if defined(MDL_INITIALIZE_CONDITIONS)
  /* Function: mdlInitializeConditions ========================================
   * Abstract:
   *    In this function, you should initialize the continuous and discrete
   *    states for your S-function block.  The initial states are placed
   *    in the state vector, ssGetContStates(S) or ssGetRealDiscStates(S).
   *    You can also perform any other initialization activities that your
   *    S-function may require. Note, this routine will be called at the
   *    start of simulation and if it is present in an enabled subsystem
   *    configured to reset states, it will be call when the enabled subsystem
   *    restarts execution to reset the states.
   */
  static void mdlInitializeConditions(SimStruct *S)
  {
  }
#endif /* MDL_INITIALIZE_CONDITIONS */



#define MDL_START  /* Change to #undef to remove function */
#if defined(MDL_START) 
  /* Function: mdlStart =======================================================
   * Abstract:
   *    This function is called once at start of model execution. If you
   *    have states that should be initialized once, this is the place
   *    to do it.
   */
  
  static int clientID=-1;
  
  
  static char firstTime=0;
  static void mdlStart(SimStruct *S) 
  {   
      //Primero Abro VREP 
      if(firstTime!=1){ //Verifico si es la primera vez que corro el programa
          firstTime=1; //indico que ya no debe ejecutarse
          char *sceneName	= mxArrayToString(ssGetSFcnParam(S,NOMBRE));
          int i=system(sceneName);
          Sleep(10000);//espero 2 seg a que se abra Vrep
      }
      
      clientID=simxGetConnectionId(clientID);
      if (clientID==-1){ //Si el servidor esta desconectado reconectar
          clientID=simxStart((simxChar*)"127.0.0.1",19997,true,true,2000,5);
          extApi_sleepMs(50);
       }
      if (clientID!=-1){  //Si el servidor esta conectado iniciar simulacion 
          if(simxStartSimulation(clientID,simx_opmode_oneshot_wait)!=simx_return_ok ){
              ssSetErrorStatus(S,"ERROR simxStartSimulation: No se pudo iniciar simulacion en VREP \n");
              return;    
          }
      }
      else{
          ssSetErrorStatus(S,"ERROR simxGetConnectionId: La conexion no esta Activa\n Verifique que VREP este abierto\n");
          return;          
      }
  }
#endif /*  MDL_START */



/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
    /*const void *u = (const void*) ssGetInputPortSignal(S,0);
    void       *y = ssGetOutputPortSignal(S,0);
    y[0] = u[0];*/
}



#define MDL_UPDATE  /* Change to #undef to remove function */
#if defined(MDL_UPDATE)
  /* Function: mdlUpdate ======================================================
   * Abstract:
   *    This function is called once for every major integration time step.
   *    Discrete states are typically updated here, but this function is useful
   *    for performing any tasks that should only take place once per
   *    integration step.
   */
  static void mdlUpdate(SimStruct *S, int_T tid)
  {
  }
#endif /* MDL_UPDATE */



#define MDL_DERIVATIVES  /* Change to #undef to remove function */
#if defined(MDL_DERIVATIVES)
  /* Function: mdlDerivatives =================================================
   * Abstract:
   *    In this function, you compute the S-function block's derivatives.
   *    The derivatives are placed in the derivative vector, ssGetdX(S).
   */
  static void mdlDerivatives(SimStruct *S)
  {
  }
#endif /* MDL_DERIVATIVES */



/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
    
    if(simxGetConnectionId(clientID)!=-1){//Re verifico conexion
        if(simxStopSimulation(clientID,simx_opmode_oneshot_wait)!=simx_return_ok ){
             printf("Eror: simxStopSimulation\n");
            return;
        }    
        simxFinish(clientID);
    } 
}


/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
