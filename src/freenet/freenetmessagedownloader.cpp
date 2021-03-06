#include "freenetmessagedownloader.h"
#include "freenetmessage.h"
#include "../stringfunctions.h"
#include "../option.h"

FreenetMessageDownloader::FreenetMessageDownloader(FreenetConnection *connection, FCPv2::Connection *fcp):IFCPConnected(fcp,connection),IFCPMessageHandler(connection,"MessageDownloader")
{
	FLIPEventSource::RegisterFLIPEventHandler(FLIPEvent::EVENT_FREENET_NEWMESSAGEEDITION,this);
	FLIPEventSource::RegisterFLIPEventHandler(FLIPEvent::EVENT_FREENET_IDENTITYFOUND,this);

	Option option;
	option.Get("MessageBase",m_messagebase);

}

FreenetMessageDownloader::~FreenetMessageDownloader()
{

}

void FreenetMessageDownloader::FCPConnected()
{

}

void FreenetMessageDownloader::FCPDisconnected()
{

}

const bool FreenetMessageDownloader::HandleFCPMessage(FCPv2::Message &message)
{
	if(message["Identifier"].find(m_fcpuniqueidentifier)==0)
	{
		std::vector<std::string> idparts;
		StringFunctions::Split(message["Identifier"],"|",idparts);

		if(message.GetName()=="AllData")
		{
			std::vector<char>::size_type datalength=0;
			StringFunctions::Convert(message["DataLength"],datalength);

			if(m_fcp->WaitForBytes(100,datalength))
			{
				std::vector<char> data;
				if(m_fcp->Receive(data,datalength))
				{
					FreenetMessage fm;

					if(FreenetMessage::TryParse(std::string(data.begin(),data.end()),fm))
					{
						// send message found event
						std::map<std::string,std::string> params;

						if(fm["type"]=="channelmessage")
						{
							params["identityid"]=idparts[1];
							StringFunctions::LowerCase(fm["channel"],params["channel"]);
							params["sentdate"]=fm["sentdate"];
							params["message"]=fm.Body();
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];

							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_NEWCHANNELMESSAGE,params));
						}
						else if(fm["type"]=="privatemessage")
						{
							params["identityid"]=idparts[1];
							params["recipient"]=fm["recipient"];
							params["sentdate"]=fm["sentdate"];
							params["encryptedmessage"]=fm.Body();
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];

							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_NEWPRIVATEMESSAGE,params));
						}
						else if(fm["type"]=="joinchannel")
						{
							params["identityid"]=idparts[1];
							StringFunctions::LowerCase(fm["channel"],params["channel"]);
							params["sentdate"]=fm["sentdate"];
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];

							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_JOINCHANNEL,params));
						}
						else if(fm["type"]=="partchannel")
						{
							params["identityid"]=idparts[1];
							StringFunctions::LowerCase(fm["channel"],params["channel"]);
							params["sentdate"]=fm["sentdate"];
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];

							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_PARTCHANNEL,params));
						}
						else if(fm["type"]=="keepalive")
						{
							params["identityid"]=idparts[1];
							params["sentdate"]=fm["sentdate"];
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];
							if(fm.Headers().find("channels")!=fm.Headers().end())
							{
								params["channels"]=fm["channels"];
							}

							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_KEEPALIVE,params));
						}
						else if(fm["type"]=="settopic")
						{
							params["identityid"]=idparts[1];
							StringFunctions::LowerCase(fm["channel"],params["channel"]);
							params["sentdate"]=fm["sentdate"];
							params["topic"]=fm.Body();
							params["insertday"]=idparts[2];
							params["edition"]=idparts[3];
							
							DispatchFLIPEvent(FLIPEvent(FLIPEvent::EVENT_FREENET_SETTOPIC,params));
						}
						else
						{
							m_log->Debug("FreenetMessageDownloader::HandleFCPMessage don't know how to handle "+fm["type"]+" for "+message["Identifier"]);
						}
					}
				}
			}

			m_log->Debug("FreenetMessageDownloader::HandleFCPMessage handled AllData for "+message["Identifier"]);

			return true;
		}
		else if(message.GetName()=="DataFound")
		{
			return true;
		}
		else if(message.GetName()=="IdentifierCollision")
		{
			return true;
		}
		else if(message.GetName()=="GetFailed")
		{
			if(message["Fatal"]=="false")
			{
				int identityid=0;
				int edition=0;

				//try again if failure count is <5
				StringFunctions::Convert(idparts[1],identityid);
				StringFunctions::Convert(idparts[3],edition);

				SQLite3DB::Statement st=m_db->Prepare("SELECT Tries FROM tblRetrievedMessageIndex WHERE IdentityID=? AND Date=? AND MessageIndex=?;");
				st.Bind(0,identityid);
				st.Bind(1,idparts[2]);
				st.Bind(2,edition);
				st.Step();
				if(st.RowReturned())
				{
					std::string publickey("");
					int tries=0;
					st.ResultInt(0,tries);

					if(tries<5)
					{
						st=m_db->Prepare("UPDATE tblRetrievedMessageIndex SET Tries=Tries+1 WHERE IdentityID=? AND Date=? AND MessageIndex=?;");
						st.Bind(0,identityid);
						st.Bind(1,idparts[2]);
						st.Bind(2,edition);
						st.Step();

						st=m_db->Prepare("SELECT PublicKey FROM tblIdentity WHERE IdentityID=?;");
						st.Bind(0,identityid);
						st.Step();
						if(st.RowReturned())
						{
							st.ResultText(0,publickey);
						}

						if(publickey=="")
						{
							m_log->Error("FreenetMessageDownloader::HandleFCPMessage public key is empty");
						}

						m_log->Debug("FreenetMessageDownloader::HandleFCPMessage GetFailed, but retrying "+message["Identifier"]);
						StartRequest(identityid,publickey,idparts[2],edition);
					}
				}

			}
			return true;
		}
	}
	return false;
}

const bool FreenetMessageDownloader::HandleFLIPEvent(const FLIPEvent &flipevent)
{
	if(flipevent.GetType()==FLIPEvent::EVENT_FREENET_NEWMESSAGEEDITION || flipevent.GetType()==FLIPEvent::EVENT_FREENET_IDENTITYFOUND)
	{
		int lastedition=0;
		int newedition=0;
		std::string publickey("");
		int identityid=0;
		std::map<std::string,std::string> params=flipevent.GetParameters();

		StringFunctions::Convert(params["identityid"],identityid);
		SQLite3DB::Statement st=m_db->Prepare("SELECT IFNULL(MAX(MessageIndex)+1,0) FROM tblRetrievedMessageIndex WHERE IdentityID=? AND Date=?;");
		st.Bind(0,identityid);
		st.Bind(1,params["date"]);
		st.Step();

		if(st.RowReturned())
		{
			st.ResultInt(0,lastedition);

			if(flipevent.GetType()==FLIPEvent::EVENT_FREENET_NEWMESSAGEEDITION)
			{
				StringFunctions::Convert(params["edition"],newedition);
				m_log->Trace("FreenetMessageDownloader::HandleFLIPEvent EVENT_FREENET_NEWMESSAGEEDITION "+params["edition"]+" for "+params["identityid"]);
			}
			else if(flipevent.GetType()==FLIPEvent::EVENT_FREENET_IDENTITYFOUND)
			{
				StringFunctions::Convert(params["lastmessageindex"],newedition);
				m_log->Trace("FreenetMessageDownloader::HandleFLIPEvent EVENT_FREENET_IDENTITYFOUND "+params["lastmessageindex"]+" for "+params["identityid"]);
			}

			st=m_db->Prepare("SELECT PublicKey FROM tblIdentity WHERE IdentityID=?;");
			st.Bind(0,params["identityid"]);
			st.Step();

			if(st.RowReturned())
			{
				st.ResultText(0,publickey);
			}

			if(publickey=="")
			{
				m_log->Error("FreenetMessageDownloader::HandleFLIPEvent public key is empty");
			}

			st=m_db->Prepare("INSERT INTO tblRetrievedMessageIndex(IdentityID,Date,MessageIndex) VALUES(?,?,?);");
			for(int i=lastedition; i<=newedition; i++)
			{
				st.Bind(0,params["identityid"]);
				st.Bind(1,params["date"]);
				st.Bind(2,i);
				st.Step();
				st.Reset();

				StartRequest(identityid,publickey,params["date"],i);
			}
		}

	}
	return false;
}

void FreenetMessageDownloader::StartRequest(const int identityid, const std::string &publickey, const std::string &date, const int edition)
{
	if(publickey!="")
	{
		std::string identityidstr("");
		std::string editionstr("0");

		StringFunctions::Convert(identityid,identityidstr);
		StringFunctions::Convert(edition,editionstr);

		FCPv2::Message mess("ClientGet");
		mess["URI"]="SSK@"+publickey.substr(4)+m_messagebase+"|"+date+"|Message-"+editionstr;
		mess["Identifier"]=m_fcpuniqueidentifier+"|"+identityidstr+"|"+date+"|"+editionstr+"|"+mess["URI"];
		mess["RealTimeFlag"]="true";
		mess["ReturnType"]="direct";
		mess["MaxSize"]="32768";

		m_fcp->Send(mess);

		m_log->Debug("FreenetMessageDownloader::StartRequest started request for "+mess["Identifier"]);
	}
}
