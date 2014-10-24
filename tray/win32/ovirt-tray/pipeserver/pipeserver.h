#include <windows.h>
#include <Sddl.h>
#include <string>
#include <vector>
#include <process.h>
#include "message.h"
#ifdef PIPESERVER_EXPORTS
#define PIPESERVER_API __declspec(dllexport)
#else
#define PIPESERVER_API __declspec(dllimport)
#endif

inline void InitializeSecurityAttribute(LPSECURITY_ATTRIBUTES sa) {	
	sa->nLength = sizeof(*sa);
	sa->bInheritHandle = FALSE;
	ConvertStringSecurityDescriptorToSecurityDescriptor(
		TEXT("S:(ML;;NW;;;LW)D:(A;;0x12019f;;;WD)"), 
		SDDL_REVISION_1, &sa->lpSecurityDescriptor, 0);
}

struct PipeServer;
struct PipeConnection;
extern "C" {
	typedef void (*OnConnected)(PipeServer*, PipeConnection *, DWORD, void *);
	typedef void (*OnClientMessage)(PipeConnection *, Message*, void *);
	typedef void (*OnClientError)(PipeConnection *, INT32, void *);
	typedef void (*OnError)(PipeServer *, INT32, void *);
	typedef void (*OnMessageCompleted)(PipeConnection *, UINT32, INT32, void *);

    PIPESERVER_API PipeServer * PipeServer_NewWithThread(
                                    void * userData,
                                    OnConnected onConnected,
                                    OnError onError,
                                    OnClientMessage onClientMessage,
                                    OnClientError onClientError);
	PIPESERVER_API PipeServer * PipeServer_New(
	                                void * userData,
	                                OnConnected onConnected,
	                                OnError onError,
	                                OnClientMessage onClientMessage,
	                                OnClientError onClientError);
    PIPESERVER_API void PipeServer_Start(PipeServer *, TCHAR const * name);
	PIPESERVER_API void PipeServer_Stop(PipeServer *);
	PIPESERVER_API void PipeServer_Free(PipeServer *);
	
	PIPESERVER_API INT32 PipeConnection_Send(
                             PipeConnection * connection,
				             Message * message,
				             void *userData,
				             OnMessageCompleted onCompletion);

	PIPESERVER_API void PipeConnection_Close(PipeConnection *);
	PIPESERVER_API void PipeConnection_Free(PipeConnection *);
};