#ifndef TEXTCHANNEL_HXX
#define TEXTCHANNEL_HXX

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <TelepathyQt/BaseChannel>

#include "Connection.hxx"
#include "TelepathyParameters.hxx"

using namespace std;
using namespace resip;

namespace tr
{
   class TextChannel;
   class Connection;
   
   typedef Tp::SharedPtr<TextChannel> TextChannelPtr;

   class TextChannel : public Tp::BaseChannelTextType, public TelepathyParameters
   {
      Q_OBJECT
   public:
      static TextChannelPtr create(Tp::BaseChannel *baseChannel, const QVariantMap& parameters, QString selfIdentifier);
      void onMessageReceived(const SipMessage& message, uint senderHandle);
      
   protected:
      TextChannel(Tp::BaseChannel *baseChannel, const QVariantMap& parameters, QString selfIdentifier);
      QString sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);
      // void setChatState(uint state, Tp::DBusError *error);
      void messageAcknowledged(const QString &messageId);
      // void processReceivedMessage(const QXmppMessage &message, uint senderHandle, const QString &senderID);

      // virtual bool sendQXmppMessage(QXmppMessage &message);
      // virtual QString targetJid() const;
      // virtual QString selfJid() const;

   protected:
      Tp::BaseChannelMessagesInterfacePtr mMessagesInterface;
      Tp::BaseChannelChatStateInterfacePtr mChatStateInterface;

      uint mTargetHandle;
      QString mTargetIdentifier;
      QString mSelfIdentifier;

   private:
      resip::Data mRealm;
      resip::Data mAuthUser;
      resip::Data mPassword;
   };

}

#endif
