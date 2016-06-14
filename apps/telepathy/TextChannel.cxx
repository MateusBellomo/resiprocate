#include <TelepathyQt/ProtocolParameterList>

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/Profile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"

#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/stack/PlainContents.hxx"


#include "TextChannel.hxx"
#include "Connection.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

typedef Tp::SharedPtr<tr::TextChannel> TextChannelPtr;

class ClientMessageHandler : public ClientPagerMessageHandler
{
public:
   ClientMessageHandler()
      : finished(false),
        successful(false)
   {
   };

   virtual void onSuccess(ClientPagerMessageHandle, const SipMessage& status)
   {
      InfoLog(<<"ClientMessageHandler::onSuccess\n");
      successful = true;
      finished = true;
   }

   virtual void onFailure(ClientPagerMessageHandle, const SipMessage& status, std::auto_ptr<Contents> contents)
   {
      ErrLog(<<"ClientMessageHandler::onFailure\n");

      ErrLog(<< "Message rcv: "  << *contents << "\n");
      
      successful = false;
      finished = true;
   }

   bool isFinished() { return finished; };
   bool isSuccessful() { return successful; };

private:
   bool finished;
   bool successful;
};

class ServerMessageHandler : public ServerPagerMessageHandler
{
public:
   ServerMessageHandler() : _rcvd(false) {};
   bool isRcvd() { return _rcvd; };
   virtual void onMessageArrived(ServerPagerMessageHandle handle, const SipMessage& message)
   {
      //cout << "Message rcv: "  << message << "\n";
	
      SharedPtr<SipMessage> ok = handle->accept();
      handle->send(ok);

      Contents *body = message.getContents();
      cout << "Message rcv: "  << *body << "\n";

      _rcvd = true;
   }
private:
   bool _rcvd;
};

tr::TextChannel::TextChannel(Tp::BaseChannel *baseChannel, const QVariantMap& parameters, QString selfIdentifier)
   : Tp::BaseChannelTextType(baseChannel),
     TelepathyParameters(parameters),
     mTargetHandle(baseChannel->targetHandle()),
     mTargetIdentifier(baseChannel->targetID()),
     mSelfIdentifier(selfIdentifier)
{
   QStringList supportedContentTypes = QStringList() << QLatin1String("text/plain");
   Tp::UIntList messageTypes = Tp::UIntList() << Tp::ChannelTextMessageTypeNormal
					      << Tp::ChannelTextMessageTypeDeliveryReport;

   uint messagePartSupportFlags = Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead;
   uint deliveryReportingSupport = Tp::DeliveryReportingSupportFlagReceiveSuccesses | Tp::DeliveryReportingSupportFlagReceiveRead;

   setMessageAcknowledgedCallback(Tp::memFun(this, &TextChannel::messageAcknowledged));

   mMessagesInterface = Tp::BaseChannelMessagesInterface::create(this,
								 supportedContentTypes,
								 messageTypes,
								 messagePartSupportFlags,
								 deliveryReportingSupport);

   baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMessagesInterface));
   mMessagesInterface->setSendMessageCallback(Tp::memFun(this, &TextChannel::sendMessage));

   mChatStateInterface = Tp::BaseChannelChatStateInterface::create();
   // mChatStateInterface->setSetChatStateCallback(Tp::memFun(this, &TextChannel::setChatState));
   baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mChatStateInterface));
   
   mRealm = realm();
   // mAuthUser = "sip:"+authUser();
   mAuthUser = authUser();
   mPassword = password();
}

TextChannelPtr tr::TextChannel::create(Tp::BaseChannel *baseChannel, const QVariantMap& parameters, QString selfIdentifier)
{
   return tr::TextChannelPtr(new tr::TextChannel(baseChannel, parameters, selfIdentifier));
}

QString tr::TextChannel::sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
      
   int port = 5060;

   // InfoLog(<< "log: from: " << from << ", to: " << to << ", port: " << port << "\n");
   // InfoLog(<< "user: " << user << ", passwd: " << passwd << ", realm: " << realm << "\n");

   SharedPtr<MasterProfile> profile(new MasterProfile);
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());

   SipStack clientStack;
   DialogUsageManager clientDum(clientStack);
   clientDum.addTransport(TCP, port);
   clientDum.setMasterProfile(profile);

   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->setDefaultRegistrationTime(70);
   clientDum.getMasterProfile()->addSupportedMethod(MESSAGE);
   clientDum.getMasterProfile()->addSupportedMimeType(MESSAGE, Mime("text", "plain"));
   ClientMessageHandler *cmh = new ClientMessageHandler();
   clientDum.setClientPagerMessageHandler(cmh);


//   NameAddr naFrom(from.c_str());
   string from = mSelfIdentifier.toUtf8().constData();
   from = "sip:" + from;
   NameAddr naFrom(from.c_str());
   StackLog(<<" naFrom = " << naFrom );

   profile->setDefaultFrom(naFrom);
   profile->setDigestCredential(mRealm, mAuthUser, mPassword);


   
   InfoLog(<< "Sending MESSAGE\n");
//   NameAddr naTo(to.c_str());
   string to = mTargetIdentifier.toUtf8().constData();
   to = "sip:" + to;
   NameAddr naTo(to.c_str());
   StackLog(<<" naTo = " << naTo );

   ClientPagerMessageHandle cpmh = clientDum.makePagerMessage(naTo);


   QString content;
   foreach ( const Tp::MessagePart& part, messageParts )
   {
      if ( part.count(QLatin1String("content-type")) &&
	   part.value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain") &&
	   part.count(QLatin1String("content")) )
      {
	 content = part.value(QLatin1String("content")).variant().toString();
	 break;
      }
   }
   Data messageBody(content.toUtf8().data());
   
   auto_ptr<Contents> msgContent(new PlainContents(messageBody));
   cpmh.get()->page(msgContent);

   // Event loop - stack will invoke callbacks in our app
   while( !cmh->isFinished() )
   {
      clientStack.process(100);
      while( clientDum.process() );
   }

   if( !cmh->isSuccessful() )
   {
      ErrLog(<< "Message delivery failed, aborting");
      // TODO: look for the proper error to set here
      error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Invalid handle(s)"));
      return "";
   }

   // TODO: find out what is opaque token https://telepathy.freedesktop.org/spec/Channel_Interface_Messages.html#Method:SendMessage
   return "token";
}

void tr::TextChannel::onMessageReceived(const SipMessage& message, uint senderHandle)
{

}

void tr::TextChannel::messageAcknowledged(const QString &messageId)
{

}
