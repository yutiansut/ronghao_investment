#pragma once

class data_manager;
class file_receiver : public E15_HttpClient {
public:
	file_receiver(data_manager *data_mgr) : m_data_mgr(data_mgr) {}
	virtual ~file_receiver() {}

public:
	int connect_file_server();
	int request_file_by_url(const std::string& url);
	virtual void OnResponed(const char * url,int status,E15_ValueTable *& header,E15_String *& data,E15_Id * session_id);

private:
	data_manager *m_data_mgr;
	std::map<std::string, std::string> m_ini_so;
};

//#define RUN_AS_CLIENT

class strategy_manager;
#ifdef RUN_AS_CLIENT
class data_manager : public E15_Client {
#else
class data_manager : public E15_Server {
#endif
public:
	data_manager(strategy_manager *data_mgr);
	virtual ~data_manager();

public:
	int connect_data_server();
	void terminate_connect();

	int request_subscribe_by_id(E15_StringArray& sa, int start, int end, int interval);
	int request_subscribe_all(int start, int end, int interval);
	int request_unsubscribe_by_id(E15_StringArray& sa);
	int request_unsubscribe_all();

	void send_instruction(const std::string& instrument_id, const order_instruction& instruction);
	void load_strategy(const std::vector<std::string>& args);

public:
	/* server callback */
#ifdef RUN_AS_CLIENT
	virtual void OnLoginOk(E15_ClientInfo * user,E15_String *& json);
	virtual int OnLogout(E15_ClientInfo * user);
	virtual void OnNotify(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json);

	virtual int OnLoginFailed(E15_ClientInfo * user,int status,const char * errmsg) { return 2000; }
	virtual void OnRequest(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json) {}
	virtual void OnResponse(E15_ClientInfo * user,E15_ClientMsg * cmd,E15_String *& json) {}
#else
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json);
	virtual int OnClose(E15_ServerInfo * info);
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data);
#endif

private:
	void insert_date_interval(E15_ValueTable& vt, unsigned int start, unsigned int end, unsigned int interval);

private:
	std::mutex m_mtx_for_trade_request;
	E15_Id m_proxy_id;
	std::map<std::string, E15_Id> m_role_id;
	strategy_manager *m_data_mgr;
	file_receiver *m_file_receiver;
};