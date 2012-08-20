//==============================================================================
// Brief   : MIH-User
// Authors : Bruno Santos <bsantos@av.it.pt>
//------------------------------------------------------------------------------
// ODTONE - Open Dot Twenty One
//
// Copyright (C) 2009-2012 Universidade Aveiro
// Copyright (C) 2009-2012 Instituto de Telecomunicações - Pólo Aveiro
//
// This software is distributed under a license. The full license
// agreement can be found in the file LICENSE in this distribution.
// This software may not be copied, modified, sold or distributed
// other than expressed in the named license agreement.
//
// This software is distributed without any warranty.
//==============================================================================

#include <odtone/base.hpp>
#include <odtone/debug.hpp>
#include <odtone/logger.hpp>
#include <odtone/mih/request.hpp>
#include <odtone/mih/response.hpp>
#include <odtone/mih/indication.hpp>
#include <odtone/mih/confirm.hpp>
#include <odtone/mih/tlv_types.hpp>
#include <odtone/sap/user.hpp>

#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <pthread.h>	//yan-21/02/2012

///////////////////////////////////////////////////////////////////////////////

static const char* const kConf_MIH_Commands = "user.commands";
///////////////////////////////////////////////////////////////////////////////

namespace po = boost::program_options;

using odtone::uint;
using odtone::ushort;

odtone::logger log_("mih_usr", std::cout);

///////////////////////////////////////////////////////////////////////////////
void __trim(odtone::mih::octet_string &str, const char chr)
{
	str.erase(std::remove(str.begin(), str.end(), chr), str.end());
}

/**
 * Parse supported commands.
 *
 * @param cfg Configuration options.
 * @return A bitmap mapping the supported commands.
 */
boost::optional<odtone::mih::mih_cmd_list> parse_supported_commands(const odtone::mih::config &cfg)
{
	using namespace boost;

	odtone::mih::mih_cmd_list commands;
	bool has_cmd = false;

	std::map<std::string, odtone::mih::mih_cmd_list_enum> enum_map;
	enum_map["mih_link_get_parameters"]       = odtone::mih::mih_cmd_link_get_parameters;
	enum_map["mih_link_configure_thresholds"] = odtone::mih::mih_cmd_link_configure_thresholds;
	enum_map["mih_link_actions"]              = odtone::mih::mih_cmd_link_actions;
	enum_map["mih_net_ho_candidate_query"]    = odtone::mih::mih_cmd_net_ho_candidate_query;
	enum_map["mih_net_ho_commit"]             = odtone::mih::mih_cmd_net_ho_commit;
	enum_map["mih_n2n_ho_query_resources"]    = odtone::mih::mih_cmd_n2n_ho_query_resources;
	enum_map["mih_n2n_ho_commit"]             = odtone::mih::mih_cmd_n2n_ho_commit;
	enum_map["mih_n2n_ho_complete"]           = odtone::mih::mih_cmd_n2n_ho_complete;
	enum_map["mih_mn_ho_candidate_query"]     = odtone::mih::mih_cmd_mn_ho_candidate_query;
	enum_map["mih_mn_ho_commit"]              = odtone::mih::mih_cmd_mn_ho_commit;
	enum_map["mih_mn_ho_complete"]            = odtone::mih::mih_cmd_mn_ho_complete;

	std::string tmp = cfg.get<std::string>(kConf_MIH_Commands);
	__trim(tmp, ' ');

	char_separator<char> sep1(",");
	tokenizer< char_separator<char> > list_tokens(tmp, sep1);

	BOOST_FOREACH(std::string str, list_tokens) {
		if(enum_map.find(str) != enum_map.end()) {
			commands.set((odtone::mih::mih_cmd_list_enum) enum_map[str]);
			has_cmd = true;
		}
	}

	boost::optional<odtone::mih::mih_cmd_list> supp_cmd;
	if(has_cmd)
		supp_cmd = commands;

	return supp_cmd;
}

///////////////////////////////////////////////////////////////////////////////
//yan-start-23/02/2012
class link_info {
public:
	link_info() {}
	link_info(odtone::mih::link_tuple_id &li, odtone::mih::octet_string &name)
		: _lid(li), _ifname(name)
	{}

	void lid(odtone::mih::link_tuple_id &li) {_lid = li;}
	void ifname(odtone::mih::octet_string &name) {_ifname = name;}

	odtone::mih::link_tuple_id lid() {return _lid;}
	odtone::mih::octet_string ifname() {return _ifname;}
private:
	odtone::mih::link_tuple_id _lid;
	odtone::mih::octet_string _ifname;
};

odtone::mih::octet_string mih_get_ifname(odtone::mih::link_tuple_id &lid)
{
	odtone::mih::octet_string cmd;
	cmd = (boost::get<odtone::mih::mac_addr>(lid.addr)).address();
	if (cmd == "")
	{
		cmd = "ifconfig | grep \"Point-to-Point Protocol";
	}
	else
	{
		cmd = "ifconfig | grep \"HWaddr " + cmd;
	}
	cmd += "\" | awk \'{print $1}\'";
	FILE *fp = popen(cmd.c_str(), "r");
	char name[20];
	if (fp != NULL)
	{
		if (fgets(name, 20, fp) != NULL)
		{
			pclose(fp);
			int i;
			for (i = 0; name[i] != '\n' && name[i] != '\0'; ++i);
			name[i] = '\0';
			return odtone::mih::octet_string(name);
		}
		pclose(fp);
	}
	return "";
}

enum req_info_type
{
	REQ_INFO_ALL,
	REQ_INFO_COST,
	REQ_INFO_BANDWIDTH
};

class client_info	//info of clients that wait for the information response
{
public:
	client_info() {}
	client_info(struct sockaddr_in addr, size_t addrlen, req_info_type type)
		: cli_addr(addr), addr_len(addrlen), req_type(type)
	{}
	client_info(struct sockaddr_in addr, size_t addrlen)//, int appid)
		: cli_addr(addr), addr_len(addrlen)//, app_id(appid)
	{}
	//int get_app_id();
	int get_port();
	void send_msg(int sockfd, const char *msg);
	void respond(int sockfd, odtone::mih::octet_string xml_info);
private:
	//cli_addr needs to be copied into client_info
	//in case it is changed elsewhere or deleted after that function
	struct sockaddr_in cli_addr;
	size_t addr_len;
	//int app_id;	//the registration app_id
	req_info_type req_type; //type of information it requests
};

int client_info::get_port()
{
	return ntohs(cli_addr.sin_port);
}

void client_info::send_msg(int sockfd, const char *msg)
{
	log_(0, "Message sent to port ", get_port());
	if (sendto(sockfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&cli_addr, addr_len) < 0)
	{
		log_(0, "client_info failed to send msg");
	}
}

void client_info::respond(int sockfd, odtone::mih::octet_string xml_info)
{
	odtone::mih::octet_string response;
	int begin, end;

	//std::cout << "respond!" << std::endl;
	switch (req_type)
	{
	case REQ_INFO_ALL:
		begin = xml_info.find("<result>") + 9;
		end = xml_info.find("</result>") - 5;
		break;
	case REQ_INFO_COST:
		//<binding name="cost_value"><literal>20</literal></binding>
		begin = xml_info.find("<binding name=\"cost_value\"") + 36;
		end = xml_info.find("</literal>", begin);
		break;
	case REQ_INFO_BANDWIDTH:
		//<binding name="bandwidth"><literal>56</literal></binding>
		begin = xml_info.find("<binding name=\"bandwidth\"") + 35;
		end = xml_info.find("</literal>", begin);
		break;
	default:
		begin = 0;
		end = 0;
		break;
	}
	
	if (end == -1)
	{
		begin = 0;
		end = 0;
	}
	response = xml_info.substr(begin, end - begin);
	log_(0, "The response is ", response);
	send_msg(sockfd, response.c_str());	
}
//yan-end
///////////////////////////////////////////////////////////////////////////////
/**
 * This class provides an implementation of an IEEE 802.21 MIH-User.
 */
class mih_user : boost::noncopyable {
public:
	/**
	 * Construct the MIH-User.
	 *
	 * @param cfg Configuration options.
	 * @param io The io_service object that the MIH-User will use to
	 * dispatch handlers for any asynchronous operations performed on the socket. 
	 */
	mih_user(const odtone::mih::config& cfg, boost::asio::io_service& io);

	/**
	 * Destruct the MIH-User.
	 */
	~mih_user();

	//yan-start-22/02/2012
	void list_links();
	bool exist_link(odtone::mih::octet_string ifname);
	odtone::mih::link_type get_link_type(unsigned int i);
	odtone::mih::octet_string get_link_addr(unsigned int i);
	odtone::mih::octet_string get_link_name(unsigned int i);

	void set_sockfd(int sockfd) {_sockfd = sockfd;}
	bool register_app(client_info* client);
	bool deregister_app(int portno);
	void information_request(odtone::mih::octet_string ifname, client_info* client);
	//yan-end

protected:
	/**
	 * User registration handler.
	 *
	 * @param cfg Configuration options.
	 * @param ec Error Code.
	 */
	void user_reg_handler(const odtone::mih::config& cfg, const boost::system::error_code& ec);

	/**
	 * Default MIH event handler.
	 *
	 * @param msg Received message.
	 * @param ec Error code.
	 */
	void event_handler(odtone::mih::message& msg, const boost::system::error_code& ec);

	/**
	 * Capability Discover handler.
	 *
	 * @param msg Received message.
	 * @param ec Error Code.
	 */
	void capability_discover_confirm(odtone::mih::message& msg, const boost::system::error_code& ec);

	/**
	 * Event subscribe handler.
	 *
	 * @param msg Received message.
	 * @param ec Error Code.
	 */
	void event_subscribe_response(odtone::mih::message& msg, const boost::system::error_code& ec);

	//yan - start - information services functions
	void information_response(odtone::mih::message& msg, const boost::system::error_code& ec);
	//yan - end

private:
	odtone::sap::user _mihf;	/**< User SAP helper.		*/
	odtone::mih::id   _mihfid;	/**< MIHF destination ID.	*/

	int _sockfd;				//yan - the sockfd of nm listener socket
	std::vector<link_info> _mihlnks;	//yan - the list of links
	std::vector<client_info> _waitlist;	//yan - the list of waiting clients
	std::vector<client_info> _applist;	//yan - the list of registered applist
	//odtone::mih::link_tuple_id _mihlid[256];	//yan-22/02/2012
	//int _num_links;	//yan-22/02/2012
};

/**
 * Construct the MIH-User.
 *
 * @param cfg Configuration options.
 * @param io The io_service object that the MIH-User will use to
 * dispatch handlers for any asynchronous operations performed on the socket. 
 */
mih_user::mih_user(const odtone::mih::config& cfg, boost::asio::io_service& io)
	: _mihf(cfg, io, boost::bind(&mih_user::event_handler, this, _1, _2))
{
	odtone::mih::message m;
	boost::optional<odtone::mih::mih_cmd_list> supp_cmd = parse_supported_commands(cfg);

	m << odtone::mih::indication(odtone::mih::indication::user_register)
	    & odtone::mih::tlv_command_list(supp_cmd);
	m.destination(odtone::mih::id("local-mihf"));

	_mihf.async_send(m, boost::bind(&mih_user::user_reg_handler, this, boost::cref(cfg), _2));
}


/**
 * Destruct the MIH-User.
 */
mih_user::~mih_user()
{
}

/**
 * User registration handler.
 *
 * @param cfg Configuration options.
 * @param ec Error Code.
 */
void mih_user::user_reg_handler(const odtone::mih::config& cfg, const boost::system::error_code& ec)
{
	log_(0, "MIH-User register result: ", ec.message());

	odtone::mih::message msg;

	odtone::mih::octet_string destination = cfg.get<odtone::mih::octet_string>(odtone::sap::kConf_MIH_SAP_dest);
	_mihfid.assign(destination.c_str());

	//
	// Let's fire a capability discover request to get things moving
	//
	msg << odtone::mih::request(odtone::mih::request::capability_discover, _mihfid);

	_mihf.async_send(msg, boost::bind(&mih_user::capability_discover_confirm, this, _1, _2));

	log_(0, "MIH-User has sent a Capability_Discover.request towards its local MIHF");
}

/**
 * Default MIH event handler.
 *
 * @param msg Received message.
 * @param ec Error code.
 */
void mih_user::event_handler(odtone::mih::message& msg, const boost::system::error_code& ec)
{
	//yan-start-17/02/2012
	//odtone::mih::message m;
	//m << odtone::mih::indication(odtone::mih::indication::link_get_parameters);
	//_mihf.sync_send(m);
	//yan-end

	if (ec) {
		log_(0, __FUNCTION__, " error: ", ec.message());
		return;
	}

	//yan-start-17/02/2012
	odtone::mih::link_tuple_id lid;
	odtone::mih::octet_string ifname;
	std::vector<link_info>::iterator it;
	std::vector<client_info>::iterator it_app;
	bool exist = false;

	msg >> odtone::mih::indication()
		& odtone::mih::tlv_link_identifier(lid);

	switch (msg.mid()) {
	case odtone::mih::indication::link_up:
		log_(0, "MIH-User has received a local event \"link_up\"");
		//log_(0, lid.addr);
		//_mihlid[_num_links] = lid;
		//++_num_links;
		exist = false;
		for (it = _mihlnks.begin(); it != _mihlnks.end(); ++it)
                {
                        if (it->lid() == lid)
                        {
                                exist = true;
                                break;
                        }
                }
		if (!exist)
		{
			ifname = mih_get_ifname(lid);
			_mihlnks.push_back(link_info(lid, ifname));
			for (it_app = _applist.begin(); it_app != _applist.end(); ++it_app)
			{	//notify the applications that interface change occurs
				it_app->send_msg(_sockfd, "");
			}
		}
		break;

	case odtone::mih::indication::link_down:
		log_(0, "MIH-User has received a local event \"link_down\"");
		for (it = _mihlnks.begin(); it != _mihlnks.end(); ++it)
		{
			if (it->lid() == lid)
			{
				_mihlnks.erase(it);
				for (it_app = _applist.begin(); it_app != _applist.end(); ++it_app)
				{	//notify the applications that interface change occurs
					it_app->send_msg(_sockfd, "");
				}
				break;
			}
		}
		/*int i;
		for (i = 0; i < _num_links; ++i)
		{
			if (_mihlid[i] == lid)
			{
				--_num_links;
				break;
			}
		}
		for (i = i + 1; i <= _num_links; ++i)
		{
			_mihlid[i-1] = _mihlid[i];
		}*/
		break;
	//yan-end

	case odtone::mih::indication::link_detected:
		log_(0, "MIH-User has received a local event \"link_detected\"");
		break;

	case odtone::mih::indication::link_going_down:
		log_(0, "MIH-User has received a local event \"link_going_down\"");
		break;

	case odtone::mih::indication::link_handover_imminent:
		log_(0, "MIH-User has received a local event \"link_handover_imminent\"");
		break;
	case odtone::mih::indication::link_handover_complete:
		log_(0, "MIH-User has received a local event \"link_handover_complete\"");
		break;
	}
}

/**
 * Capability Discover handler.
 *
 * @param msg Received message.
 * @param ec Error Code.
 */
void mih_user::capability_discover_confirm(odtone::mih::message& msg, const boost::system::error_code& ec)
{
	if (ec) {
		log_(0, __FUNCTION__, " error: ", ec.message());
		return;
	}

	odtone::mih::status st;
	boost::optional<odtone::mih::net_type_addr_list> ntal;
	boost::optional<odtone::mih::mih_evt_list> evt;

	msg >> odtone::mih::confirm()
		& odtone::mih::tlv_status(st)
		& odtone::mih::tlv_net_type_addr_list(ntal)
		& odtone::mih::tlv_event_list(evt);

	log_(0, "MIH-User has received a Capability_Discover.response with status ",
			st.get(), " and the following capabilities:");

	if (ntal) {
		for (odtone::mih::net_type_addr_list::iterator i = ntal->begin(); i != ntal->end(); ++i)
			log_(0,  *i);

	} else {
		log_(0,  "none");
	}

	//
	// event subscription
	//
	// For every interface the MIHF sent in the
	// Capability_Discover.response send an Event_Subscribe.request
	// for all availabe events
	//
	if (ntal && evt) {
		for (odtone::mih::net_type_addr_list::iterator i = ntal->begin(); i != ntal->end(); ++i) {
			odtone::mih::message req;
			odtone::mih::link_tuple_id li;

			if (i->nettype.link.which() == 1)
				{
					li.addr = i->addr;
					li.type = boost::get<odtone::mih::link_type>(i->nettype.link);

					//yan-start-23/02/2012
					//_mihlid[_num_links] = li;
					//++_num_links;
					odtone::mih::octet_string ifname = mih_get_ifname(li);
					if (ifname != "")
					{
						_mihlnks.push_back(link_info(li, ifname));
					}
					//yan-end

					req << odtone::mih::request(odtone::mih::request::event_subscribe, _mihfid)
						& odtone::mih::tlv_link_identifier(li)
						& odtone::mih::tlv_event_list(evt);
					req.destination(msg.source());

					_mihf.async_send(req, boost::bind(&mih_user::event_subscribe_response, this, _1, _2));

					log_(0, "MIH-User has sent Event_Subscribe.request to ", req.destination().to_string());
				}
		}
	}
}

/**
 * Event subscribe handler.
 *
 * @param msg Received message.
 * @param ec Error Code.
 */
void mih_user::event_subscribe_response(odtone::mih::message& msg, const boost::system::error_code& ec)
{
	log_(0, __FUNCTION__, "(", msg.tid(), ")");

	if (ec) {
		log_(0, __FUNCTION__, " error: ", ec.message());
		return;
	}

	odtone::mih::status st;

	msg >> odtone::mih::response()
		& odtone::mih::tlv_status(st);

	log_(0, "status: ", st.get());
}

//yan-start-21/02/2012
void mih_user::list_links()
{
	//log_(0, "No\tType\tAddress\n");
	std::cout << "No.\tName\tAddress" << std::endl;
	for (unsigned int i = 0; i < _mihlnks.size(); ++i)
	{
		//log_(0, i, '\t', _mihlid[i].type, '\t');
		std::cout << (i + 1) << '\t' << get_link_name(i);
		/*
		switch (get_link_type(i).get())
		{
		case odtone::mih::link_type_gsm: std::cout << "GSM\t";	break;
		case odtone::mih::link_type_gprs: std::cout << "GPRS\t";	break;
		case odtone::mih::link_type_edge: std::cout << "EDGE\t";	break;

		case odtone::mih::link_type_ethernet: std::cout << "Ethernet";	break;

		case odtone::mih::link_type_wireless_other: std::cout << "Wireless";	break;
		case odtone::mih::link_type_802_11:
			if (get_link_addr(i) == "")
			{
				std::cout << "LTE\t";
			}
			else
			{
				std::cout << "802.11\t";
			}
			break;

		case odtone::mih::link_type_cdma2000: std::cout << "CDMA2000";	break;
		case odtone::mih::link_type_umts: std::cout << "UMTS\t";	break;
		case odtone::mih::link_type_cdma2000_hrpd: std::cout << "CDMA2000-HRPD";	break;

		case odtone::mih::link_type_802_16: std::cout << "802.16\t";	break;
		case odtone::mih::link_type_802_20: std::cout << "802.20\t";	break;
		case odtone::mih::link_type_802_22: std::cout << "802.22\t";	break;
		}*/
 		std::cout << '\t' << get_link_addr(i) << std::endl;
	}
}
//yan-end

//yan-start-22/03/2012
bool mih_user::exist_link(odtone::mih::octet_string ifname)
{
	bool found = false;
	for (unsigned int i = 0; i < _mihlnks.size(); ++i)
	{
		//std::cout << get_link_name(i) << ifname << get_link_name(i).compare(ifname) << std::endl;
		if (get_link_name(i).compare(ifname) == 0)
		{
			found = true;
			break;
		}
	}
	return found;
}
//yan-end

//yan-start-21/02/2012
odtone::mih::link_type mih_user::get_link_type(unsigned int i)
{
	if (i < 0 || i >= _mihlnks.size())
	{
		return (odtone::mih::link_type());
	}
	//return _mihlid[i].type;
	return _mihlnks.at(i).lid().type;
}
//yan-end

//yan-start-21/02/2012
odtone::mih::octet_string mih_user::get_link_addr(unsigned int i)
{
	if (i < 0 || i >= _mihlnks.size())
	{
		return "";
	}
	//return (boost::get<odtone::mih::mac_addr>(_mihlid[i].addr)).address();
	return (boost::get<odtone::mih::mac_addr>(_mihlnks.at(i).lid().addr)).address();
}
//yan-end

//yan-start-23/02/2012
odtone::mih::octet_string mih_user::get_link_name(unsigned int i)
{
	if (i < 0 || i >= _mihlnks.size())
	{
		return "";
	}
	//return (boost::get<odtone::mih::mac_addr>(_mihlid[i].addr)).address();
	return _mihlnks.at(i).ifname();
}
//yan-end

//yan-start-4/19/2012
bool mih_user::register_app(client_info* client)
{
	bool existed = false;

	for (std::vector<client_info>::iterator it = _applist.begin();
		it != _applist.end(); ++it)
	{
		//log_(0, it->get_port(), client->get_port());
		if (it->get_port() == client->get_port())
		{
			existed = true;
			break;
		}
	}

	if (existed)
	{
		log_(0, "Application (Listening port: ",
			client->get_port(), ") has re-registered");
		return false;
	}

	_applist.push_back(*client);
	log_(0, "Application (Listening port: ",
		client->get_port(), ") has registered");

	return true;
}

bool mih_user::deregister_app(int portno)
{
	log_(0, "Application (Listening port: ",
		portno, ") has de-registered");
	for (std::vector<client_info>::iterator it = _applist.begin();
		it != _applist.end(); ++it)
	{
		if (it->get_port() == portno)
		{
			_applist.erase(it);
			break;
		}
	}
	return true;
}
//yan-end

//yan-start-4/4/2012
void mih_user::information_request(odtone::mih::octet_string ifname, client_info* client)
{
        odtone::mih::message p;

        odtone::mih::iq_rdf_data query;
        odtone::mih::iq_rdf_data_list query_list;

        p.source(odtone::mih::id("user"));
        p.destination(odtone::mih::id("mihf1"));

        // Below is an example of IS query. The MIH client queries
        // available networks for a specific location identified by
        // cellular cell ID “800”. The client requires the server to
        // return the network_id and link_type of all available
        // networks for this location.
	odtone::mih::octet_string str;
	str = "PREFIX mihbasic: <http://www.mih.org/2006/09/rdf-basic-schema#> \
        PREFIX xsd: <http://www.w3.org/2001/XMLSchema#>                 \
        SELECT ?cost_unit ?cost_value ?cost_curr ?bandwidth             \
        WHERE {                                                         \
          	  ?network mihbasic:ie_network_id \"" + ifname + "\" .  \
		  ?network mihbasic:ie_cost ?cost .                     \
                  ?cost mihbasic:cost_unit ?cost_unit .                 \
                  ?cost mihbasic:cost_value ?cost_value .               \
                  ?cost mihbasic:cost_curr ?cost_curr .                 \
                  ?network mihbasic:bandwidth ?bandwidth .		\
        }";
	query._data.assign(str);

        // other queries can be added in the request
        query_list.push_back(query);

        // create and send a Get_Information request
        p << odtone::mih::request(odtone::mih::request::get_information)
                & odtone::mih::tlv_info_query_rdf_data_list(query_list);

	//printf("%d %d\n", (int)p.opcode(), (int)odtone::mih::operation::request);

        _mihf.async_send(p, boost::bind(&mih_user::information_response, this, _1, _2));

	//add client to waitlist
	if (client)
		_waitlist.push_back(*client);
}

void mih_user::information_response(odtone::mih::message& msg, const boost::system::error_code& ec)
{
	odtone::mih::status st;
	odtone::mih::ir_rdf_data_list rsp_list;
	unsigned int i;
	//std::cout << "Information received!!!" << std::endl;

	msg >> odtone::mih::response()
		& odtone::mih::tlv_status(st)
		& odtone::mih::tlv_info_resp_rdf_data_list(rsp_list);

        log_(0, "MIH-User has received a get_information.response with status ",
                        st.get(), " and the following responses:");

	if (st != odtone::mih::status_success)
	{
		return;
	}

	for (i = 0; i < rsp_list.size(); ++i)
	{
		//odtone::mih::ir_rdf_data rsp = rsp_list[i];
		//odtone::mih::ie_network_id id;
		//rsp.input() & odtone::mih::tlv_ie_network_id(id);
		//log_(0, "Network ID: ", id);

		//std::cout << rsp_list[i]._data << std::endl;	//debug - show xml information
		for (std::vector<client_info>::iterator it = _waitlist.begin();
			it != _waitlist.end(); ++it)
		{
			it->respond(_sockfd, rsp_list[i]._data);
		}
		_waitlist.clear();
	}
}
//end-yan

//yan-start-23/02/2012
int send_mip6d(odtone::mih::octet_string cmd)
{
	int sockfd;
        int n, i;
        struct sockaddr_in serv_addr;
	//odtone::mih::octet_string cmd;
        char buf[256];

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
                log_(0, "MIPv6: Failed to create socket");
                return -1;
        }

        serv_addr.sin_family = AF_INET;
        inet_aton("127.0.0.1", &serv_addr.sin_addr);
        serv_addr.sin_port = htons(7777);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
                log_(0, "MIPv6: Failed to establish connection");
                return -1;
        }

	n = read(sockfd, buf, 255);
        n = write(sockfd, cmd.c_str(), cmd.length() + 1);
        if (n < 0)
        {
                log_(0, "MIPv6: Failed to send command via socket");
		return -1;
        }
	else
	{
		log_(0, "Command sent to MIPv6 daemon: ", cmd);
	}

        n = read(sockfd, buf, 255);
	if (n < 0)
	{
		log_(0, "MIPv6: Failed to receive result");
		return -1;
	}
	for (i = 0; i < n && buf[i] != '\n'; ++i);
	buf[i] = 0;
	log_(0, "MIPv6: ", buf);

        close(sockfd);
	return 0;
}
//yan-end

//yan-start-10/04/2012
int send_hipd(odtone::mih::octet_string cmd)
{
	int sockfd;
        int n;
	int i;
	unsigned int len;
        struct sockaddr_in serv_addr;
	struct timeval tv;
	//odtone::mih::octet_string cmd;
        char buf[256];

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
                log_(0, "HIP: Failed to create socket");
                return -1;
        }

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&tv, sizeof(struct timeval)) < 0)
	{
		log_(0, "HIP: Failed to set timeout");
		return -1;
	}

        serv_addr.sin_family = AF_INET;
        inet_aton("127.0.0.1", &serv_addr.sin_addr);
        serv_addr.sin_port = htons(7776);

	//n = read(sockfd, buf, 255);
        n = sendto(sockfd, cmd.c_str(), cmd.length() + 1, 0,
		(struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (n < 0)
        {
                log_(0, "HIP: Failed to send command via socket");
		return -1;
        }
	else
	{
		log_(0, "Command sent to HIP daemon: ", cmd);
	}

	len = sizeof(serv_addr);
        n = recvfrom(sockfd, buf, 255, 0, (struct sockaddr *)&serv_addr, &len);
	if (n < 0)
	{
		log_(0, "HIP daemon not responding");
		return -1;
	}
	for (i = 0; i < n && buf[i] != '\n'; ++i);
	buf[i] = 0;
	log_(0, "HIP: ", buf);

        close(sockfd);
	return 0;
}
//yan-end

//yan-start-22/03/2012
void *socket_handler(void *arg)
{
	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	mih_user *p_usr = (mih_user *) arg;

	fd_set rfds;
        //struct timeval tv;
	int len;
	//int appid;
	unsigned int addrlen = sizeof(cli_addr);
	char buf[256];

	//std::cout << "1" << std::endl;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		log_(0, "failed to create datagram socket");
		return NULL;
	}
	p_usr->set_sockfd(sockfd);
	//std::cout << "2" << std::endl;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(18752);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		log_(0, "failed to bind socket");
		return NULL;
	}
	//std::cout << "3" << std::endl;
/*
	if (listen(sockfd, 10) < 0)
	{
		log_(0, "failed to listen");
	}*/
	
        //tv.tv_sec = 0;
        //tv.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	//std::cout << "4" << std::endl;
	while (true)
	{
		if (select(sockfd + 1, &rfds, 0, 0, 0) < 0)
		{
			log_(0, "failed to select");
			break;
		}

		if (FD_ISSET(sockfd, &rfds))
		{
			//std::cout << "a";
			if ((len = recvfrom(sockfd, buf, 255, 0, (struct sockaddr *)&cli_addr, &addrlen)) < 1)
			{
				log_(0, "Socket handler: failed to receive msg or bad msg format");
				continue;
			}
			if (buf[0] != 'r' && buf[0] != 'd' && len < 3)
			{
				log_(0, "Socket handler: bad msg format");
				continue;
			}
			buf[len] = '\0';
			log_(0, "Message received by socket handler: ", buf);
	
			switch (buf[0])
			{
			case 'e':
				if (p_usr->exist_link(odtone::mih::octet_string(buf + 2)))
				{
					//std::cout << "up" << std::endl;
					if (sendto(sockfd, "up", 3, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
					{
						log_(0, "Socket handler: failed to send msg");
					}
				}
				else
				{
					//std::cout << "down" << std::endl;
					if (sendto(sockfd, "down", 5, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
					{
						log_(0, "Socket handler: failed to send msg");
					}
				}
				break;

			case 'i':
				p_usr->information_request(buf + 2,
					new client_info(cli_addr, addrlen, REQ_INFO_ALL));
				break;

			case 'c':
				p_usr->information_request(buf + 2,
					new client_info(cli_addr, addrlen, REQ_INFO_COST));
				break;

			case 'b':
				p_usr->information_request(buf + 2,
					new client_info(cli_addr, addrlen, REQ_INFO_BANDWIDTH));
				break;

			case 'h':	//set interface for hip
				if (buf[2] == 'p' && buf[3] == 'p' && buf[4] == 'p') {// && IP_VERSION == 6) {
					send_hipd("pref sixxs");	//ipv6 tunnel for ppp0
				} //else if (buf[2] == 'e' && buf[3] == 't' && buf[4] == 'h') {// && IP_VERSION == 6) {
				//	send_hipd("pref he-ipv6");	//ipv6 tunnel for eth0
				//}
				else {
					send_hipd("pref " + odtone::mih::octet_string(buf + 2));
				}
				if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
				{
					log_(0, "Socket handler: failed to send ack");
				}
				break;

			case 'g':	//confirm using that interface for hip
				//send_hipd("pref " + odtone::mih::octet_string(buf + 2));
				if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
                                {
                                        log_(0, "Socket handler: failed to send ack");
                                }
                                break;

			case 'm':
				if (buf[2] == 'p' && buf[3] == 'p' && buf[4] == 'p') {// && IP_VERSION == 6) {
					send_mip6d("pref sixxs");	//ipv6 tunnel for ppp0
				} //else if (buf[2] == 'e' && buf[3] == 't' && buf[4] == 'h') {// && IP_VERSION == 6) {
				//	send_mip6d("pref he-ipv6");	//ipv6 tunnel for eth0
				//} 
				else {
					send_mip6d("pref " + odtone::mih::octet_string(buf + 2));
				}
				if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
				{
					log_(0, "Socket handler: failed to send ack");
				}
				break;

			case 'r':
				//appid = atoi(buf + 2);
				if (p_usr->register_app(new client_info(cli_addr, addrlen)))//, appid)))
				{	//cli_addr needs to be copied into client_info
					//in case it is changed elsewhere or deleted after that function
					if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
						log_(0, "Socket handler: failed to send ask");
				}
				else
				{
					if (sendto(sockfd, "nak", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
						log_(0, "Socket handler: failed to send nak");
				}
				break;

			case 'd':
				//appid = atoi(buf + 2);
				if (p_usr->deregister_app(ntohs(cli_addr.sin_port)))
				{
					if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
						log_(0, "Socket handler: failed to send ask");
				}
				else
				{
					if (sendto(sockfd, "nak", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
						log_(0, "Socket handler: failed to send nak");
				}
				break;

			default:
				break;
			}	
		}
	}

	close(sockfd);

	//std::cout << "end" << std::endl;
	return NULL;
}

void *keyboard_handler(void *arg)
{
	int i;
	char buf[256];
	mih_user *p_usr = (mih_user *) arg;
	odtone::mih::link_type type;
	odtone::mih::octet_string cmd, ifname;

	sleep(1);

	while (true)
	{
		std::cout << "> ";
		std::cin.getline(buf, 256);

		FILE *fp = popen("netstat -rn | grep ^0.0.0.0 | awk \'{print $8}\'", "r");	//ipv4
		//char temp[20] = {0};

		switch (buf[0])
		{
		case 'l':
			p_usr->list_links();
			break;
		case 'h':
		case 'm':
		case 'c':
			i = atoi(buf + 1) - 1;
			ifname = p_usr->get_link_name(i);
			if (ifname == "")
			{
				std::cout << "Number out of range\n> " << std::flush;
			}
			else
			{
				if (buf[0] != 'm')	//hip
				{/*
					//cmd = "sudo route add -net 0.0.0.0 netmask 0.0.0.0 dev " + ifname;
					if (fgets(temp, 20, fp) != NULL)
					{
						int j;
						for (j = 0; temp[j] != '\n' && temp[j] != '\0'; ++j);
						temp[j] = '\0';
						//std::cout << ifname.c_str() << ' ' << temp << std::endl;
						if (!strcmp(ifname.c_str(), temp)) //same interface, no need to change
						{
							std::cout << "HIP: Already on " << temp << std::endl;
						}
						else
						{
							cmd = cmd + " && sudo route del default dev " + temp ;
							system(cmd.c_str());

							//cmd = "sudo ip route flush table sine && sudo ip route add table sine default dev " + ifname;
							//system(cmd.c_str());

							std::cout << "HIP: Interface changed" << std::endl;
						}
					}
					else	//Apr 4th
					{
						system(cmd.c_str());
						std::cout << "HIP: Interface changed" << std::endl;
					}*/
					//std::cout << cmd << std::endl;
					//hip_pref(ifname);
					if (ifname[0] == 'p' && ifname[1] == 'p' && ifname[2] == 'p') {// && IP_VERSION == 6) {
						send_hipd("pref sixxs");	//ipv6 tunnel for ppp0
					} //else if (ifname[0] == 'e' && ifname[1] == 't' && ifname[2] == 'h') {// && IP_VERSION == 6) {
					//	send_hipd("pref he-ipv6");	//ipv6 tunnel for eth0
					//} 
					else {
						send_hipd("pref " + ifname);
					}
				}

				//ipv6
				/* disable ipv6
				cmd = "cat /proc/sys/net/ipv6/conf/" + ifname + "/disable_ipv6";
				fp = popen(cmd.c_str(), "r");
				if (fgets(temp, 20, fp) != NULL && temp[0] == '0')
				{
						std::cout << "Already on " << ifname << std::endl;
                                                break;
                                }
				system("sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1");
				cmd = "sudo sysctl -w net.ipv6.conf." + ifname + ".disable_ipv6=0";
				system(cmd.c_str());
				std::cout << "Handover complete" << std::endl;
				*/
				if (buf[0] != 'h')	//mipv6
				{
					if (ifname[0] == 'p' && ifname[1] == 'p' && ifname[2] == 'p') {// && IP_VERSION == 6) {
						send_mip6d("pref sixxs");	//ipv6 tunnel for ppp0
					} //else if (ifname[0] == 'e' && ifname[1] == 't' && ifname[2] == 'h') {// && IP_VERSION == 6) {
					//	send_mip6d("pref he-ipv6");	//ipv6 tunnel for eth0
					//}
					else {
						send_mip6d("pref " + ifname);
					}
				}
			}
			break;
		case 'i':
			i = atoi(buf + 1) - 1;
                        ifname = p_usr->get_link_name(i);
			if (ifname == "")
                        {
                                std::cout << "Number out of range\n> " << std::flush;
                        }
			else
			{
				p_usr->information_request(ifname, NULL);
			}
			break;
		case 'e':
			if (strcmp(buf, "encrypt on") == 0)
			{
				send_hipd("eoff 0");
			}
			else if (strcmp(buf, "encrypt off") == 0)
			{
				send_hipd("eoff 1");
			}
			else
			{
				std::cout << "Wrong command!" << std::endl;
			}
			break;
		case 'q':
			exit(0);
			break;
		default:
			break;
		}
		
		if (fp != NULL)
		{
			pclose(fp);
		}
	}

	return NULL;
}
//yan-end

int main(int argc, char** argv)
{
	pthread_t keyboard_thread;	//yan
	pthread_t socket_thread;	//yan

	odtone::setup_crash_handler();

	try {
		boost::asio::io_service ios;

		// declare MIH Usr available options
		po::options_description desc(odtone::mih::octet_string("MIH Usr Configuration"));
		desc.add_options()
			("help", "Display configuration options")
			(odtone::sap::kConf_File, po::value<std::string>()->default_value("mih_usr.conf"), "Configuration file")
			(odtone::sap::kConf_Receive_Buffer_Len, po::value<uint>()->default_value(4096), "Receive buffer length")
			(odtone::sap::kConf_Port, po::value<ushort>()->default_value(1234), "Listening port")
			(odtone::sap::kConf_MIH_SAP_id, po::value<std::string>()->default_value("user"), "MIH-User ID")
			(kConf_MIH_Commands, po::value<std::string>()->default_value(""), "MIH-User supported commands")
			(odtone::sap::kConf_MIHF_Ip, po::value<std::string>()->default_value("127.0.0.1"), "Local MIHF IP address")			
			(odtone::sap::kConf_MIHF_Local_Port, po::value<ushort>()->default_value(1025), "Local MIHF communication port")
			(odtone::sap::kConf_MIH_SAP_dest, po::value<std::string>()->default_value(""), "MIHF destination");

		odtone::mih::config cfg(desc);
		cfg.parse(argc, argv, odtone::sap::kConf_File);

		if (cfg.help()) {
			std::cerr << desc << std::endl;
			return EXIT_SUCCESS;
		}

		mih_user usr(cfg, ios);

		pthread_create(&keyboard_thread, NULL, keyboard_handler, (void *) &usr);	//yan-21/02/2012
		pthread_create(&socket_thread, NULL, socket_handler, (void *) &usr);	//yan-22/03/2012

		ios.run();

	} catch(std::exception& e) {
		log_(0, "exception: ", e.what());
	}
}

// EOF ////////////////////////////////////////////////////////////////////////
