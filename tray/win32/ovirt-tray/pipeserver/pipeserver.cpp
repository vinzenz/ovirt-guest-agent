// pipeserver.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "pipeserver.h"
#include "named_pipe.h"
#include "message.h"
#include "bson.h"

struct ClientSendHandler {
    PipeConnection * connection;
    std::vector<BYTE> * message;
    void * userData;
    OnMessageCompleted onCompletion;
    ClientSendHandler(PipeConnection * connection, Message * message, void * userData, OnMessageCompleted onCompletion)
    : connection(connection)
    , message(new (std::nothrow) std::vector<BYTE>())
    , userData(userData)
    , onCompletion(onCompletion)        
    {
        ConvertToBuffer(message);
    }
private:
    void AppendEntry(MessageEntry & entry) {
        size_t idx = message->size();
        message->insert(message->end(), 4, 0);        
        *reinterpret_cast<UINT32*>(&(*message)[idx]) = entry.Length;
        message->insert(message->end(), entry.Value, entry.Value + entry.Length);
    }
    
    void AppendField(MessageField & field) {
        AppendEntry(field.Key);
        AppendEntry(field.Value);
    }
    
    void ConvertToBuffer(Message * m) {
        message->resize(sizeof(UINT32)* 2, 0);        
        for(UINT32 i = 0; i < m->FieldCount; ++i) {
            AppendField(m->Fields[i]);
        }
        UINT32 * hdr = reinterpret_cast<UINT32*>(&(*message)[0]);
        hdr[0] = UINT32(message->size() - (sizeof(UINT32) * 2));
        hdr[1] = m->FieldCount;
    }
public:    
    BYTE const * buffer() const {
        if(bufferLength() == 0) {
            return 0;
        }
        return reinterpret_cast<BYTE const *>(&(*message)[0]);
    }
    
    DWORD bufferLength() const {
        if(!message) {
            return 0;
        }
        return DWORD(message->size());
    }
    
    void operator()(DWORD err, DWORD trans) {
        delete message;
        message = 0;
        if(onCompletion) {
            onCompletion(connection, trans, HRESULT_FROM_WIN32(err), userData);
        }
    }
};

struct AcceptResultHandler {
    PipeServer * server;
    PipeConnection * connection;
    AcceptResultHandler(PipeServer *, PipeConnection *);
    
    void operator()(DWORD err, DWORD trans);
};


struct PipeConnection {
    NamedPipeStream conn;
    PipeServer * server;
    
    PipeConnection(PipeServer * server);
    ~PipeConnection(){
        Close();
    }   
    void Close();
};

struct PipeServer {
    bool started;
    Service service;
    NamedPipeAcceptor acceptor;
    SECURITY_ATTRIBUTES securityAttributes;
    void * userData;
    OnConnected onConnected;
    OnError onError;
    OnClientMessage onClientMessage;
    OnClientError onClientError;

    PipeServer(bool);
    ~PipeServer();
    void AcceptNext();
    void Start(TCHAR const * name);
    void Stop();
    void ScheduleRead(PipeConnection * connection);
};

struct ClientMessageReadHandler {
    PipeConnection * connection;
    PipeServer * server;
    Message * message;
    char * buffer;
    enum {
        MSGSTATE_HEADER,
        MSGSTATE_BODY,
        MSGSTATE_DONE
    } state;

    ClientMessageReadHandler(ClientMessageReadHandler const & other)
    : connection(other.connection)
    , server(other.server)
    , message(other.message)
    , buffer(other.buffer)
    , state(other.state)
    {}
    
    ClientMessageReadHandler & operator=(ClientMessageReadHandler o) {
        connection = o.connection;
        server = o.server;
        message = o.message;
        buffer = o.buffer;
        state = o.state;
        return *this;
    }

    ClientMessageReadHandler(PipeServer * server, PipeConnection * connection)
    : server(server)
    , connection(connection)
    , message(new (std::nothrow) Message())
    , state(MSGSTATE_HEADER)
    {
        detail::map_value foo_map;
        detail::list_value foo_list;
        detail::binary_view foo_bin;
        detail::string_view foo_str;
        detail::int32_value foo_int32;
        detail::int64_value foo_int64;
        detail::byte_value  foo_byte;
        DWORD result = connection->conn.AsyncRead(reinterpret_cast<BYTE*>(message), sizeof(UINT32) * 2, *this);
        if(result != NO_ERROR) {
            server->onClientError(connection, result, server->userData);
        }
    }    

    void operator()(DWORD err, DWORD bytes) {
        if(state == MSGSTATE_HEADER) {
            OnHeaderReceived(err, bytes);
        }
        else if(state == MSGSTATE_BODY) {
            OnBodyReceived(err, bytes);
        }        
    }    

    bool ReadSize(char const *& buffer, UINT32 & bytesLeft, UINT32 & target) {
        if(bytesLeft < sizeof(UINT32)) {
            return false;
        }
        bytesLeft -= sizeof(UINT32);
        target = *reinterpret_cast<UINT32 const*>(buffer);
        buffer += sizeof(UINT32);
        return true;
    } 
    
    bool ReadString(char const *& buffer, UINT32 length, UINT32 & bytesLeft, char const *& target, std::vector<std::string> & store) {
        if(bytesLeft < length) {
            return false;
        }
        bytesLeft -= length;
        store.push_back(std::string(buffer, buffer + length));
        buffer += length;
        target = store.back().c_str();
        return true;
    }
    
    bool ReadEntry(char const *& buffer, UINT32 & bytesLeft, MessageEntry & entry, std::vector<std::string> & store) {
        if(ReadSize(buffer, bytesLeft, entry.Length)) {
            if(ReadString(buffer, entry.Length, bytesLeft, entry.Value, store)) {
                return true;
            }
        }       
        return false; 
    }

    void OnBodyReceived(DWORD err, DWORD bytes) {
        if(err != NO_ERROR) {
            if(server->onClientError) {
                server->onClientError(connection, err, server->userData);
            }
        }
        else if(bytes != message->BodySize) {
            // We're still missing data, reschedule read
        }
        else {
            state = MSGSTATE_DONE;
            buffer[message->BodySize-1] = 0; // Enforce 0 termination
            
            std::vector<MessageField> fieldStore(message->FieldCount);
            std::vector<std::string> stringStore;
            stringStore.reserve(message->FieldCount * 2);
            char const * ptr = buffer;
            UINT32 bytesLeft = bytes;
            for(size_t i = 0; i < fieldStore.size(); ++i) {                
                if(!ReadEntry(ptr, bytesLeft, fieldStore[i].Key, stringStore)) {
                    break;
                }
                if(!ReadEntry(ptr, bytesLeft, fieldStore[i].Value, stringStore)) {
                    break;
                }
            }            
            message->Fields = &fieldStore[0];
            server->onClientMessage(connection, message, server->userData);
            server->ScheduleRead(connection);
        }
        delete[] buffer;
        delete message;
    }

    void OnHeaderReceived(DWORD err, DWORD bytes) {
        if(err != NO_ERROR) {
            // handle error
            if(server->onClientError) {
                server->onClientError(connection, err, server->userData);
            }
        }
        else if(bytes != (sizeof(UINT32) * 2)) {
            // We're still missing data, reschedule read
        }
        else {
            buffer = new (std::nothrow) char[message->BodySize]();
            state = MSGSTATE_BODY;
            connection->conn.AsyncRead(reinterpret_cast<BYTE*>(buffer), message->BodySize, *this);
        }
    }
};

AcceptResultHandler::AcceptResultHandler(PipeServer * server, PipeConnection * connection)
: server(server)
, connection(connection)
{}

void AcceptResultHandler::operator ()(DWORD err, DWORD trans)
{
    if(err != 0) {
        if(server->onError) {
            server->onError(server, HRESULT_FROM_WIN32(err), server->userData);
        }
    }
    else {
        ULONG pid = 0;
        ::GetNamedPipeClientProcessId(connection->conn.Native(), &pid);
        server->onConnected(server, connection, pid, server->userData);
        server->ScheduleRead(connection);
    }
    server->AcceptNext();
}

PipeServer::PipeServer(bool start)
: started(start)
, service()
, acceptor(service)
, securityAttributes()
{
}

PipeServer::~PipeServer() {
    Stop();
    service.Stop();
}

void PipeServer::AcceptNext() {
    PipeConnection * client = new PipeConnection(this);
    HRESULT result = S_OK;
    if(FAILED(result = HRESULT_FROM_WIN32(acceptor.AsyncAccept(client->conn, AcceptResultHandler(this, client))))) {
        if(onError) {
            onError(this, result, userData);
        }
    }
}

void PipeServer::Start(TCHAR const * name) {
    InitializeSecurityAttribute(&securityAttributes);
    HRESULT result = S_OK;
    if(FAILED(result = HRESULT_FROM_WIN32(acceptor.Open(name, &securityAttributes)))) {
        if(onError) {
            onError(this, result, userData);
        }
    }
    else {
        this->AcceptNext();
    }
    if(started) service.Start();
    else service.Run();
}    

void PipeServer::Stop() {
    service.Stop();	
    acceptor.Close();
}

void PipeServer::ScheduleRead(PipeConnection * connection) {
    ClientMessageReadHandler(this, connection);
}

PipeConnection::PipeConnection(PipeServer * server)
: conn(server->service)
, server(server)
{}    

void PipeConnection::Close(){
    conn.Close();
}


extern "C" {
    PipeServer * PipeServer_NewWithThread(
                        void * userData,
                        OnConnected onConnected,
                        OnError onError,
                        OnClientMessage onClientMessage,
                        OnClientError onClientError) {
        
        PipeServer * self = new (std::nothrow) PipeServer(true);
        if(self) {
            self->onClientError = onClientError;
            self->onClientMessage = onClientMessage;
            self->onConnected = onConnected;
            self->onError = onError;
            self->userData = userData;
        }
        return self;
    }

    PipeServer * PipeServer_New(
                        void * userData,
                        OnConnected onConnected,
                        OnError onError,
                        OnClientMessage onClientMessage,
                        OnClientError onClientError) {

            PipeServer * self = new (std::nothrow) PipeServer(false);
            if(self) {
                self->onClientError = onClientError;
                self->onClientMessage = onClientMessage;
                self->onConnected = onConnected;
                self->onError = onError;
                self->userData = userData;
            }
            return self;
    }

    void PipeServer_Start(PipeServer * self, TCHAR const * name) {
        self->Start(name);        
    }

    void PipeServer_Stop(PipeServer * self) {
        self->Stop();
    }

    void PipeServer_Free(PipeServer * self) {
        delete self;
    }

	INT32 PipeConnection_Send(
                        PipeConnection * connection,
                        Message * message,
                        void *userData,
                        OnMessageCompleted onCompletion) {        
        if(!message) {
            return ERROR_INVALID_PARAMETER;
        }
        ClientSendHandler handler(connection, message, userData, onCompletion);
        connection->conn.AsyncWrite(handler.buffer(), handler.bufferLength(), handler);
        return 0;
    }
    
    void PipeConnection_Close(PipeConnection * self) {
        self->Close();
    }

    void PipeConnection_Free(PipeConnection * self) {
        delete self;
    }
};
