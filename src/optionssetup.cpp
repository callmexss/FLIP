#include "optionssetup.h"
#include "db/sqlite3db.h"
#include "global.h"

#include <string>
#include <sstream>

void SetupDefaultOptions(SQLite3DB::DB *db)
{
	// OptionValue should always be inserted as a string, even if the option really isn't a string - just to keep the field data type consistent

	db->Execute("BEGIN;");

	std::ostringstream tempstr;	// must set tempstr to "" between db inserts
	SQLite3DB::Statement st=db->Prepare("INSERT INTO tblOption(Option,OptionValue) VALUES(?,?);");
	SQLite3DB::Statement upd=db->Prepare("UPDATE tblOption SET Section=?, SortOrder=?, ValidValues=?, OptionDescription=?, DisplayType=?, DisplayParam1=?, DisplayParam2=?, Mode=? WHERE Option=?;");
	int order=0;

	// LogLevel
	tempstr.str("");
	tempstr << LogFile::LOGLEVEL_DEBUG;
	st.Bind(0,"LogLevel");
	st.Bind(1,tempstr.str());
	st.Step();
	st.Reset();
	upd.Bind(0,"Program");
	upd.Bind(1,order++);
	upd.Bind(2,"1|1 - Fatal Errors|2|2 - Critical Errors|3|3 - Errors|4|4 - Warnings|5|5 - Notices|6|6 - Informational Messages|7|7 - Debug Messages|8|8 - Trace Messages");
	upd.Bind(3,"The maximum logging level that will be written to file.  Higher levels will include all messages from the previous levels.");
	upd.Bind(4,"select");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"LogLevel");
	upd.Step();
	upd.Reset();

	st.Bind(0,"VacuumOnStartup");
	st.Bind(1,"false");
	st.Step();
	st.Reset();
	upd.Bind(0,"Program");
	upd.Bind(1,order++);
	upd.Bind(2,"true|true|false|false");
	upd.Bind(3,"VACUUM the database every time FLIP starts.  This will defragment the free space in the database and create a smaller database file.  Vacuuming the database can be CPU and disk intensive.");
	upd.Bind(4,"select");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"VacuumOnStartup");
	upd.Step();
	upd.Reset();

	st.Bind(0,"MessageBase");
	st.Bind(1,"flip");
	st.Step();
	st.Reset();
	upd.Bind(0,"Program");
	upd.Bind(1,order++);
	upd.Bind(2);
	upd.Bind(3,"A unique string shared by all clients who want to communicate with each other.  This should not be changed unless you want to create your own separate communications network.");
	upd.Bind(4,"textbox");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"advanced");
	upd.Bind(8,"MessageBase");
	upd.Step();
	upd.Reset();

	// IRCListenPort
	st.Bind(0,"IRCListenPort");
	st.Bind(1,"6667");
	st.Step();
	st.Reset();
	upd.Bind(0,"IRC Server");
	upd.Bind(1,order++);
	upd.Bind(2);
	upd.Bind(3,"The port that the IRC service will listen for incoming connections.");
	upd.Bind(4,"textbox");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"IRCListenPort");
	upd.Step();
	upd.Reset();

	// IRCBindAddresses
	st.Bind(0,"IRCBindAddresses");
	st.Bind(1,"localhost,127.0.0.1");
	st.Step();
	st.Reset();
	upd.Bind(0,"IRC Server");
	upd.Bind(1,order++);
	upd.Bind(2);
	upd.Bind(3,"A comma separated list of valid IPv4 or IPv6 addresses/hostnames that the IRC service will try to bind to.");
	upd.Bind(4,"textbox");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"IRCBindAddresses");
	upd.Step();
	upd.Reset();

	// FCPHost
	st.Bind(0,"FCPHost");
	st.Bind(1,"127.0.0.1");
	st.Step();
	st.Reset();
	upd.Bind(0,"Freenet Connection");
	upd.Bind(1,order++);
	upd.Bind(2);
	upd.Bind(3,"Host name or address of Freenet node.");
	upd.Bind(4,"textbox");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"FCPHost");
	upd.Step();
	upd.Reset();

	// FCPPort
	st.Bind(0,"FCPPort");
	st.Bind(1,"9481");
	st.Step();
	st.Reset();
	upd.Bind(0,"Freenet Connection");
	upd.Bind(1,order++);
	upd.Bind(2);
	upd.Bind(3,"The port that Freenet is listening for FCP connections on.");
	upd.Bind(4,"textbox");
	upd.Bind(5);
	upd.Bind(6);
	upd.Bind(7,"simple");
	upd.Bind(8,"FCPPort");
	upd.Step();
	upd.Reset();

	db->Execute("COMMIT;");

}