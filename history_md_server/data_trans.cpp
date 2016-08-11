#include "data_trans.h"
#include "history_mgr.h"

E15_Socket g_socket;

data_trans::data_trans(history_mgr *mgr_ptr)
:m_mgr_ptr(mgr_ptr)
,m_market(-1) {
}

int data_trans::start() {
	g_socket.Start();
	Start(&g_socket, "ini/server.ini");

	E15_Ini ini;
	ini.Read("ini/config.ini");
	ini.SetSection("role");
	m_stg_role = ini.ReadString("stg", "");
	m_dia_role = ini.ReadString("dia", "");
	return 1;
}

void data_trans::stop() {
	Stop();
	g_socket.Stop();
}

void data_trans::send_data(const std::string& ins_id, E15_String *data, int cmd) {
	E15_ServerCmd sc;
	sc.cmd = cmd;
	sc.status = MarketCodeById(ins_id.c_str());
	Notify(&m_dia_id, 0, &sc, data->c_str(), data->Length());
	delete data;
}

int data_trans::OnOpen(E15_ServerInfo * info,E15_String *& json) {
	print_thread_safe("[%x:%x] (N=%d,name=%s:role=%s) 上线\n", info->id.h,
			info->id.l, info->N, info->name, info->role);

	if (!strcmp(info->role, m_stg_role.c_str())) {
		m_stg_id = info->id;
		print_thread_safe("[history_md]连接到策略服务器(%x:%x, name=%s, role=%s)\n", info->id.h,
				info->id.l, info->name, info->role);
	}

	if (!strcmp(info->role, m_dia_role.c_str())) {
		m_dia_id = info->id;
		send_ins_info("ni1609", m_dia_id);
		print_thread_safe("[history_md]连接到指标生成服务器(%x:%x, name=%s, role=%s)\n", info->id.h,
				info->id.l, info->name, info->role);
	}
	return 1;
}

int data_trans::OnClose(E15_ServerInfo * info) {
	print_thread_safe("[%x:%x] (N=%d, name=%s:role=%s) 下线\n", info->id.h, info->id.l,
			info->N, info->name, info->role);
	return 1000;		//自动重联
}

void data_trans::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
	switch(cmd->cmd) {		//只要接到请求就相应
	case Stock_Msg_SubscribeById:
	case Stock_Msg_UnSubscribeById:
		printf("收到订阅/退订请求 id=[%x:%x] cmd=%d data=%s！\n", info->id.h, info->id.l, cmd->cmd, data->c_str());
		handle_subscribe(cmd->cmd, data);
		break;

	default:
		break;
	}
}

int data_trans::handle_subscribe(int cmd, E15_String *&data) {
	E15_ValueTable vt;
	vt.Import(data->c_str(), data->Length());
	E15_Value *v = vt.ValueS("id_list");
	if (!v) {
		print_thread_safe("[data_trans]handle subscribe by id error, not find id_list, req data len=%d\n", data->Length());
		return -1;
	}
	E15_StringArray *sa = v->GetStringArray();

	unsigned int  start, end, interval;
	if (Stock_Msg_SubscribeById == cmd) {
		v = vt.ValueS("start_date");
		start = v->GetUInt();

		v = vt.ValueS("end_date");
		end = v->GetUInt();

		v = vt.ValueS("interval");
		interval = v->GetUInt();
	}

	for (unsigned long i = 0; i < sa->Size(); ++i) {
		E15_String *s = sa->At(i);
		if (Stock_Msg_UnSubscribeById == cmd)		//取消订阅
			m_mgr_ptr->history_unsubscribe(s->c_str());
		else		//Stock_Msg_SubscribeById == cmd
			m_mgr_ptr->history_subscribe(s->c_str(), start, end, interval);
	}
	return sa->Size();
}

void data_trans::send_ins_info(const char *ins, E15_Id& id) {
	E15_ValueTable *vt = m_instrument_list.InsertTableS(ins);
	m_market = MarketCodeById(ins);
	vt->SetSI("market", m_market);
	vt->SetSS("name", ins);
	vt->SetSI("tick", 100);
	vt->SetSI("Multiple", 1);
	vt->SetSS("exchange", "sh");
	vt->SetSS("product", "ni");


	m_instrument_list.Print();
	E15_String s;
	m_instrument_list.Dump(&s);

	m_zip.zip_start(&m_instrument_buffer);
	m_zip.zip(s.c_str(), s.Length());
	m_zip.zip_end();

	if( m_instrument_buffer.Length() > 0 )
	{
		E15_ServerCmd  cmd;
		cmd.cmd = Stock_Msg_InstrumentList; //合约代码信息
		cmd.status = m_market;
		Notify(&id,0,&cmd,m_instrument_buffer.c_str(),m_instrument_buffer.Length(),1 );
	}
	m_instrument_list.Reset();
	m_instrument_buffer.Reset();
}
