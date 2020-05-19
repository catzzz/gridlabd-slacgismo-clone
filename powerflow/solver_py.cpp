/** Solver modeling
    Enable modeling of solution to improve performance
 **/

#include "gridlabd.h"

#include <stdlib.h>
#include <unistd.h>

#include "solver_py.h"

#define CONFIGNAME "solver_py.conf"
#define CONFIGPATH "/usr/local/share/gridlabd/"

static SOLVERPYTHONSTATUS solver_py_status = SPS_INIT;
char1024 solver_py_config = CONFIGPATH CONFIGNAME;
static const char *model_busdump = NULL;
static const char *model_branchdump = NULL;
static const char *model_dump_handler = NULL;
static const char *module_import_path = NULL; // path to use when importing modules
static const char *module_import_name = NULL; // module name to import (python only)
static PyObject *pModule = NULL;
static int solver_python_loglevel = -1; // -1=disable, 0 = minimal ... 9 = everything,
static FILE *solver_python_logfh = NULL;

void solver_python_log(int level, const char *format, ...)
{
	if ( (int)level <= solver_python_loglevel && solver_python_logfh != NULL )
	{
		va_list ptr;
		va_start(ptr,format);
		vfprintf(solver_python_logfh,format,ptr);
		fprintf(solver_python_logfh,"\n");
		fflush(solver_python_logfh);
		va_end(ptr);
	}
}

SOLVERPYTHONSTATUS solver_python_config (
	const char *localconfig = NULL, 
	const char *shareconfig = CONFIGPATH CONFIGNAME)
{
	const char *configname = localconfig ? localconfig : (const char*)solver_py_config;
	FILE *fp = fopen(configname,"r");
	if ( fp == NULL )
	{
		configname = shareconfig;
		fp = fopen(configname,"r");
	}
	if ( fp != NULL )
	{
		SOLVERPYTHONSTATUS status = SPS_READY;
		char line[1024];
		while ( fgets(line,sizeof(line),fp) != NULL )
		{
			char tag[1024],value[1024];
			if ( sscanf(line,"%s%s",tag,value) == 2 )
			{
				if ( tag[0] == '#' )
				{
					continue;
				}
				else if ( strcmp(tag,"logfile") == 0 )
				{
					solver_python_logfh = fopen(value,"w");
					if ( solver_python_logfh )
					{
						solver_python_log(0,"solver_python_config(configname='%s'): solver log '%s' opened ok",configname,value);
					}
					else
					{
						fprintf(stderr,"solver_python_config(configname='%s'): solver log '%s' opened failed (errno=%d, strerror='%s')\n",configname,value,errno,strerror(errno));
					}
				}
				else if ( strcmp(tag,"loglevel") == 0 )
				{
					solver_python_loglevel = atoi(value);
					solver_python_log(0,"solver_python_config(configname='%s'): solver_python_loglevel = %d",configname,solver_python_loglevel);
				}
				else if ( strcmp(tag,"busdump") == 0 )
				{
					model_busdump = strdup(value);
					solver_python_log(0,"solver_python_config(configname='%s'): model_busdump = '%s'",configname,model_busdump);
				}
				else if ( strcmp(tag,"branchdump") == 0 )
				{
					model_branchdump = strdup(value);
					solver_python_log(0,"solver_python_config(configname='%s'): model_branchdump = '%s'",configname,model_branchdump);
				}
				else if ( strcmp(tag,"on_dump") == 0 )
				{
					model_dump_handler = strdup(value);
					solver_python_log(0,"solver_python_config(configname='%s'): model_dump_handler = '%s'",configname,model_dump_handler);
				}
				else if ( strcmp(tag,"import") == 0 )
				{
					module_import_name = strdup(value);
					solver_python_log(0,"solver_python_config(configname='%s'): module_import_name = '%s'",configname,module_import_name);
				}
				else if ( strcmp(tag,"import_path") == 0 )
				{
					module_import_path = strdup(value);
					solver_python_log(0,"solver_python_config(configname='%s'): module_import_path = '%s'",configname,module_import_path);
				}
				else if ( strcmp(tag,"solver") == 0 )
				{
					if ( strcmp(value,"enable") == 0 )
					{
						status = SPS_READY;
						solver_python_log(0,"solver_python_config(configname='%s'): solver enabled",configname);
					}
					else if ( strcmp(value,"disable") == 0 )
					{
						status = SPS_DISABLED;
						solver_python_log(0,"solver_python_config(configname='%s'): solver disabled",configname);
					}
					else
					{
						fprintf(stderr,"solver_python_config(configname='%s'): tag '%s' value '%s' is invalid\n",configname,tag,value);
						status = SPS_FAILED;
					}
				}
				else
				{
					fprintf(stderr,"solver_python_config(configname='%s'): tag '%s' is not valid\n",configname,tag);
				}
			}
		}
		fclose(fp);
		fflush(stderr);
		return status;
	}
	else
	{
		return SPS_FAILED;
	}
}

int solver_python_init(void)
{
	errno = 0;
	if ( solver_py_status == SPS_INIT )
	{
		solver_py_status = solver_python_config();
		const char *status_text[] = {"INIT","READY","FAILED","DISABLED","UNKNOWN"};
		if ( (int)solver_py_status >= 0 && (int)solver_py_status < (int)(sizeof(status_text)/sizeof(status_text[0])) )
		{
			solver_python_log(0,"solver_python_init(): solver_py_status = SPS_%s",status_text[solver_py_status]);
		}
		else
		{
			solver_python_log(0,"solver_python_init(): solver_py_status unknown");
		}

		if ( solver_py_status == SPS_READY )
		{
			pModule = python_import(module_import_name,module_import_path);
			if ( pModule == NULL )
			{
				solver_python_log(0,"ERROR: solver_python_init(): module '%s' not found in path '%s'", module_import_name, module_import_name?module_import_name:"");
			}
		}
	}
	return solver_py_status == SPS_READY ? 0 : ( errno ? errno : -1 );
}

static PyObject *sync_model(void)
{
	static PyObject *pModel = NULL;
	if ( pModel == NULL )
	{
		pModel = PyDict_New();		
		// PyDict_SetItemString(pModel,"bustags",PyList_New(0));
		// PyDict_SetItemString(pModel,"busdata",PyList_New(0));
		// PyDict_SetItemString(pModel,"branchtags",PyList_New(0));
		// PyDict_SetItemString(pModel,"branchdata",PyList_New(0));
	}
	return pModel;
}

int solver_python_solve (
	unsigned int &bus_count,
	BUSDATA *&bus,
	unsigned int &branch_count,
	BRANCHDATA *&branch,
	NR_SOLVER_STRUCT *powerflow_values, 
	NRSOLVERMODE powerflow_type , 
	NR_MESHFAULT_IMPEDANCE *mesh_imped_values, 
	bool *bad_computations,
	int64 &iterations)
{
	if ( pModule )
	{
		PyObject *pModel = sync_model();
		if ( ! python_call(pModule,"solve","N",pModel) )
		{
			solver_python_log(1,"solver_python_solve(bus_count=%d,...): solve failed",bus_count);
			return -1;
		}
		else
		{
			try
			{
				// get result of last call
				PyObject *py_value = PyDict_GetItemString(pModel,"iterations");
				long result = PyLong_AsLong(py_value);
				solver_python_log(0,"solver_python_solve(bus_count=%d,...): result = %d",bus_count,result);
				return (int)result;
			}
			catch (...)
			{
				solver_python_log(0,"ERROR: solver_python_solve(bus_count=%d,...): result is not a long value",bus_count);
				return -1;
			}
		}
	}
	else
	{
		solver_python_log(0,"ERROR: solver_python_solve(bus_count=%d,...): gridlabd module not yet ready",bus_count);
		return -1;
	}
}

void solver_python_learn (
	unsigned int bus_count,
	BUSDATA *bus, 
	unsigned int branch_count, 
	BRANCHDATA *branch, 
	NR_SOLVER_STRUCT *powerflow_values, 
	NRSOLVERMODE powerflow_type , 
	NR_MESHFAULT_IMPEDANCE *mesh_imped_values, 
	bool *bad_computations,
	int64 iterations)
{
	if ( pModule )
	{
		PyObject *pModel = sync_model();
		if ( ! python_call(pModule,"learn","N",pModel) )
		{
			solver_python_log(1,"solver_python_solve(bus_count=%d,...): learn failed",bus_count);
		}
	}
}

void solver_dump(unsigned int &bus_count,
				 BUSDATA *&bus,
				 unsigned int &branch_count,
				 BRANCHDATA *&branch,
				 bool allow_generic_dumpfile)
{
	FILE *fh;
	const char *bustype[] = {"PQ","PV","SWING","SWING_PG"};
	const char *busflag[] = {"NONE","HASSOURCE","ISSOURCE","HASSOURCE|ISSOURCE"};
	char phases[256][9];
	for ( int c = 0 ; c < 256 ; c++ )
	{
		memset(phases[c],0,sizeof(phases[c]));
		if ( c&0x80 ) strcat(phases[c],"S"); // split
		if ( c&0x20 ) strcat(phases[c],"X"); // transformer
		if ( c&0x04 ) strcat(phases[c],"A"); // phase A
		if ( c&0x02 ) strcat(phases[c],"B"); // phase B
		if ( c&0x01 ) strcat(phases[c],"C"); // phase C
		if ( c&0x08 ) strcat(phases[c],"D"); // delta
		if ( c&0x10 ) strcat(phases[c],"Z"); // different children	
		if ( c&0x40 ) strcat(phases[c],"H"); // house connected node
	}
	//
	// Bus dump
	//
	if ( model_busdump == NULL && allow_generic_dumpfile )
		model_busdump = "busdump.csv";
	gl_verbose("dumping bus data to '%s'",model_busdump);
	fh = fopen(model_busdump,"w");
	if ( fh == NULL )
	{
		solver_python_log(1,"unable to open bus dumpfile '%s' for write", model_busdump);
		return;
	}
	fprintf(fh,"id,name,type,phases,origphases,busflags,"
		"vbase,mvabase,"
#define POLAR(X) #X "Am," #X "Aa," #X "Bm," #X "Ba," #X "Cm," #X "Ca,"
#define RECT(X)  #X "Ar," #X "Ai," #X "Br," #X "Bi," #X "Cr," #X "Ci,"
#define DELIM ""
#include "solver_ml_branchdump.h"
#undef POLAR
#undef RECT
#undef DELIM
		"PA,QA,PB,QB,PC,QC,"
		"YcAAr,YcAAi,YcABr,YcABi,YcACr,YcACi,YcBBr,YcBBi,YcBCr,YcBCu,YcCCr,YcCCi,"
		"YsAAr,YsAAi,YsABr,YsABi,YsACr,YsACi,YsBBr,YsBBi,YsBCr,YsBCi,YsCCr,YsCCi,"
		"YlAr,YlAi,YlBr,YlBi,YlCr,YlCi,"
		"JA0,JA1,JA2,"
		"JB0,JB1,JB2,"
		"JC0,JC1,JC2,"
		"JD0,JD1,JD2,"
		"EOL\n");
	for ( unsigned int m = 0 ; m < bus_count ; m++ )
	{
		BUSDATA *b = bus + m;
		char bustypeerr[32] = "ERROR", phaseserr[32] = "ERROR", origphaseserr[32] = "ERROR", busflagerr[32] = "ERROR";
		if ( bustype[b->type] == NULL ) snprintf(bustypeerr,sizeof(bustypeerr)-1,"ERROR:%d",b->type);
		if ( phases[b->phases] == NULL ) snprintf(phaseserr,sizeof(phaseserr)-1,"ERROR:%d",(int)b->phases);
		if ( phases[b->origphases] == NULL ) snprintf(origphaseserr,sizeof(origphaseserr)-1,"ERROR:%d",(int)b->origphases);
		if ( busflag[*(b->busflag)] == NULL ) snprintf(busflagerr,sizeof(busflagerr)-1,"ERROR:%d",(int)*(b->busflag));
		fprintf(fh,"%d,%s,%s,%s,%s,%s,"
			// Vbase and MVAbase
			"%g,%g,"
#define POLAR(X) "%g,%g,%g,%g,%g,%g," 
#define RECT(X) "%g,%g,%g,%g,%g,%g," 
#define DELIM ""
#include "solver_ml_branchdump.h"
#undef POLAR
#undef RECT
#undef DELIM
			// loads
			"%g,%g,%g,%g,%g,%g,"
			// Y matrices
			"%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,"
			"%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,"
			"%g,%g,%g,%g,%g,%g,"
			// Jacobians
			"%g,%g,%g,"
			"%g,%g,%g,"
			"%g,%g,%g,"
			"%g,%g,%g,"
			"%s\n",
			m,b->name?b->name:"",bustype[b->type]?bustype[b->type]:bustypeerr,phases[b->phases]?phases[b->phases]:phaseserr,phases[b->origphases]?phases[b->origphases]:origphaseserr,*(b->busflag)<sizeof(busflag)/sizeof(busflag[0])?busflag[*(b->busflag)]:busflagerr,
			b->volt_base > 0 ? b->volt_base : QNAN, b->mva_base > 0 ? b->mva_base : QNAN,
#define POLAR(X) (b->X)==NULL?QNAN:(b->X)[0].Mag(),(b->X)==NULL?QNAN:(b->X)[0].Ang(),(b->X)==NULL?QNAN:(b->X)[1].Mag(),(b->X)==NULL?QNAN:(b->X)[1].Ang(),(b->X)==NULL?QNAN:(b->X)[2].Mag(),(b->X)==NULL?QNAN:(b->X)[2].Ang()
#define RECT(X) (b->X)==NULL?QNAN:(b->X)[0].Re(),(b->X)==NULL?QNAN:(b->X)[0].Im(),(b->X)==NULL?QNAN:(b->X)[1].Re(),(b->X)==NULL?QNAN:(b->X)[1].Im(),(b->X)==NULL?QNAN:(b->X)[2].Re(),(b->X)==NULL?QNAN:(b->X)[2].Im()
#define DELIM ,
#include "solver_ml_branchdump.h"
#undef POLAR
#undef RECT
#undef DELIM
#define RECT(X,N) (b->X!=NULL)?(b->X[N]).Re():QNAN,(b->X!=NULL)?(b->X[N]).Im():QNAN
			b->PL[0],b->QL[0],b->PL[1],b->QL[1],b->PL[2],b->QL[2],
			RECT(full_Y,0),RECT(full_Y,1),RECT(full_Y,2),RECT(full_Y,4),RECT(full_Y,5),RECT(full_Y,8),
			RECT(full_Y_all,0),RECT(full_Y_all,1),RECT(full_Y_all,2),RECT(full_Y_all,4),RECT(full_Y_all,5),RECT(full_Y_all,8),
			RECT(full_Y_load,0),RECT(full_Y_load,1),RECT(full_Y_load,2),
			b->Jacob_A[0],b->Jacob_A[1],b->Jacob_A[2],
			b->Jacob_B[0],b->Jacob_B[1],b->Jacob_B[2],
			b->Jacob_C[0],b->Jacob_C[1],b->Jacob_C[2],
			b->Jacob_D[0],b->Jacob_D[1],b->Jacob_D[2],
			"EOL");
	}
	fclose(fh);

	//
	// Branch dump
	//
	if ( model_branchdump == NULL && allow_generic_dumpfile )
		model_branchdump = "branchdump.csv";
	gl_verbose("dumping branch data to '%s'",model_branchdump);
	fh = fopen(model_branchdump,"w");
	if ( fh == NULL )
	{
		solver_python_log(1,"unable to open branch dumpfile '%s' for write", model_branchdump);
		return;
	}
	fprintf(fh,"id,name,type,phases,origphases,faultphases,from,to,fault_link_below,v_ratio,vratio,"
		"YfromAr,YfromAi,YfromBr,YfromBi,YfromCr,YfromCi,"
		"YtoAr,YtoAi,YtoBr,YtoBi,YtoCr,YtoCi,"
		"YSfromAr,YSfromAi,YSfromBr,YSfromBi,YSfromCr,YSfromCi,"
		"YStoAr,YStoAi,YStoBr,YStoBi,YStoCr,YStoCi,"
		"EOL\n");
	for ( unsigned int n = 0 ; n < branch_count ; n++ )
	{
		BRANCHDATA *b = branch+n;
		char phaseserr[32] = "ERROR", origphaseserr[32] = "ERROR", faultphaseserr[32] = "ERROR";
		const char *branchtypes[] = {"LINE","TRIPLEX","SWITCH","FUSE","TRANSFORMER","SECTIONALIZER","RECLOSER"};
		const char *branchtype = ( b->lnk_type >= 0 && b->lnk_type < sizeof(branchtypes)/sizeof(branchtypes[0]) ? branchtypes[b->lnk_type] : "ERROR" );
		if ( phases[b->phases] == NULL ) snprintf(phaseserr,sizeof(phaseserr)-1,"ERROR:%d",(int)b->phases);
		if ( phases[b->origphases] == NULL ) snprintf(origphaseserr,sizeof(origphaseserr)-1,"ERROR:%d",(int)b->origphases);
		if ( phases[b->faultphases] == NULL ) snprintf(faultphaseserr,sizeof(faultphaseserr)-1,"ERROR:%d",(int)b->faultphases);
		fprintf(fh,"%d,%s,%s,%s,%s,%s,%d,%d,%s,%g,%g,"
			"%g,%g,%g,%g,%g,%g,"
			"%g,%g,%g,%g,%g,%g,"
			"%g,%g,%g,%g,%g,%g,"
			"%g,%g,%g,%g,%g,%g,"
			"%s\n",
			n,b->name?b->name:"",branchtype,phases[b->phases]?phases[b->phases]:phaseserr,phases[b->origphases]?phases[b->origphases]:origphaseserr,phases[b->faultphases]?phases[b->faultphases]:faultphaseserr,b->from,b->to,b->fault_link_below?"FALSE":"TRUE",b->v_ratio,b->v_ratio,
			b->Yfrom[0].Re(),b->Yfrom[0].Im(),b->Yfrom[1].Re(),b->Yfrom[1].Im(),b->Yfrom[2].Re(),b->Yfrom[2].Im(),
			b->Yto[0].Re(),b->Yto[0].Im(),b->Yto[1].Re(),b->Yto[1].Im(),b->Yto[2].Re(),b->Yto[2].Im(),
			b->YSfrom[0].Re(),b->YSfrom[0].Im(),b->YSfrom[1].Re(),b->YSfrom[1].Im(),b->YSfrom[2].Re(),b->YSfrom[2].Im(),
			b->YSto[0].Re(),b->YSto[0].Im(),b->YSto[1].Re(),b->YSto[1].Im(),b->YSto[2].Re(),b->YSto[2].Im(),
			"EOL");
	}
	fclose(fh);
	if ( model_dump_handler )
	{
		if ( strncmp(model_dump_handler,"python:",7) == 0 )
		{
			if ( ! python_call(pModule,model_dump_handler+7,NULL) )
			{
				solver_python_log(1,"model_dump_handler failed, rc = FALSE");
			}
		}
		else
		{
			int rc = system(model_dump_handler);
			if ( rc != 0 )
			{
				solver_python_log(1,"model_dump_handler failed, rc = %d", rc);
			}
		}
	}
}
