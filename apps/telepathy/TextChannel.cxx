#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

#include "TextChannel.hxx"
#include "Connection.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

typedef Tp::SharedPtr<tr::TextChannel> TextChannelPtr;

tr::TextChannel::TextChannel(tr::MyConversationManager* conversationManager, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID)
   : Tp::BaseChannelTextType(baseChannel),
     mTargetHandle(baseChannel->targetHandle()),
     mTargetID(baseChannel->targetID()),
     mSelfHandle(selfHandle),
     mSelfID(selfID),
     mConversationManager(conversationManager)
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
}

TextChannelPtr
tr::TextChannel::create(tr::MyConversationManager* conversationManager, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID)
{
   return tr::TextChannelPtr(new tr::TextChannel(conversationManager, baseChannel, selfHandle, selfID));
}

QString
tr::TextChannel::sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
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

   string to = mTargetID.toUtf8().constData();
   to = "sip:" + to;
   NameAddr naTo(to.c_str());

   const char* callId = mConversationManager->sendMessage(naTo, messageBody, Mime("text", "plain"));

   return QString(callId);
}

void
tr::TextChannel::processReceivedMessage(const resip::SipMessage& message, uint senderHandle, const QString& senderID)
{
   Tp::MessagePartList body;
   Tp::MessagePart text;
   text[QLatin1String("content-type")] = QDBusVariant(QLatin1String("text/plain"));

   QString content = message.getContents()->getBodyData().c_str();
   text[QLatin1String("content")] = QDBusVariant(content);
   body << text;
   InfoLog(<<"mbellomo content = " << message.getContents()->getBodyData().c_str());
   qDebug() << "TextChannel::processReceivedMessage() senderHandle = " << senderHandle << " senderID = " << senderID;
   
   Tp::MessagePartList partList;
   Tp::MessagePart header;

   const QString token = message.header(h_CallId).value().c_str();
   header[QLatin1String("message-token")] = QDBusVariant(token);
   header[QLatin1String("message-type")] = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
   header[QLatin1String("message-sent")] = QDBusVariant(message.getCreatedTimeMicroSec());

   uint currentTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
   header[QLatin1String("message-received")] = QDBusVariant(currentTimestamp);
   header[QLatin1String("message-sender")] = QDBusVariant(senderHandle);
   header[QLatin1String("message-sender-id")] = QDBusVariant(senderID);
   header[QLatin1String("sender-nickname")] = QDBusVariant(senderID);

   
   partList << header << body;
   addReceivedMessage(partList);
}

void
tr::TextChannel::messageAcknowledged(const QString &messageId)
{
   qDebug() << "TextChannel::messageAcknowledged() not implemented" << endl;
}
