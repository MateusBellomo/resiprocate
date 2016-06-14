#ifndef TEXTCHANNEL_HXX
#define TEXTCHANNEL_HXX

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <TelepathyQt/BaseChannel>

#include "Connection.hxx"

using namespace std;
using namespace resip;

namespace tr
{

class TextChannel;
class Connection;
   
typedef Tp::SharedPtr<TextChannel> TextChannelPtr;

class TextChannel : public Tp::BaseChannelTextType
{
   Q_OBJECT
public:
   static TextChannelPtr create(tr::MyConversationManager* conversationManager, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID);
   void processReceivedMessage(const resip::SipMessage &message, uint senderHandle, const QString& senderIdentifier);
   
protected:
   TextChannel(tr::MyConversationManager* conversationManager, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID);
   QString sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);
   void messageAcknowledged(const QString &messageId);

protected:
   Tp::BaseChannelMessagesInterfacePtr mMessagesInterface;
   Tp::BaseChannelChatStateInterfacePtr mChatStateInterface;
   
   uint mTargetHandle;
   QString mTargetID;
   uint mSelfHandle;
   QString mSelfID;

private:
   tr::MyConversationManager* mConversationManager;
};
   
}

#endif
