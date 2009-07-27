//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "asjit.h"
#include "as_jit_arm.h"

#define TESTNAME "asJITTest"
static const char *script =
"int TestInt(int a, int b, int c)          \n"
"{                                         \n"
"    int ret = 0;                          \n"
"    for (int i = 0; i < 250000; i++)       \n"
"        for (int j = 0; j < 100; j++)     \n"
"        {                                 \n"
"           ret += a*b+i*j;                \n"
"           ret += c * 2;                  \n"
"        }                                 \n"
"    return ret;                           \n"
"}                                         \n";

extern "C"
{
int __aeabi_idiv(int a, int b)
{
    return a/b;
}
}
int main(int argc, char ** argv)
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.15.0             : 0.4222 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

    asIJITCompiler *jit = new asCJitArm(engine);
//    engine->SetJITCompiler(jit);
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script, strlen(script), 0);
	mod->Build();

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(mod->GetFunctionIdByDecl("int TestInt(int a, int b, int c)"));
    ctx->SetArgDWord(0, 3);
    ctx->SetArgDWord(1, 5);
    ctx->SetArgDWord(2, 2);

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

	time = GetSystemTimer() - time;

	if( r != 0 )
	{
		printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("Script exception\n");
			asIScriptFunction *func = engine->GetFunctionDescriptorById(ctx->GetExceptionFunction());
			printf("Func: %s\n", func->GetName());
			printf("Line: %d\n", ctx->GetExceptionLineNumber());
			printf("Desc: %s\n", ctx->GetExceptionString());
		}
	}
	else
	{
		printf("Time = %f secs\n", time);
		printf("returned: %d\n", ctx->GetReturnDWord());
	}


	ctx->Release();
	engine->Release();
    delete jit;
    return r;
}
