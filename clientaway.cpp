#include <string>

#include <znc/Client.h>
#include <znc/IRCNetwork.h>
#include <znc/main.h>
#include <znc/Modules.h>

#if (VERSION_MAJOR < 1) || (VERSION_MAJOR == 1 && VERSION_MINOR < 7)
#error The clientaway module requires ZNC version 1.7.0 or later.
#endif

class ClientAway : public CModule {
public:
    MODCONSTRUCTOR(ClientAway) {}

    bool AllClientsAway() {
        for (CClient* client : GetNetwork()->GetClients()) {
            if (!client->IsAway()) {
                return false;
            }
        }
        return true;
    }

    CMessage CreateAwayRequest(bool isAway) {
        VCString params;
        if (isAway) {
            params.emplace_back("Away");
        }
        CMessage msg(
            GetNetwork()->GetIRCNick().GetNickMask(),
            "AWAY",
            params
        );
        return msg;
    }

    CMessage CreateAwayResponse(bool isAway) {
        auto line = std::string(":irc.znc.in ")
            .append(std::to_string(isAway ? 306 : 305))
            .append(" ")
            .append(GetClient()->GetNick())
            .append(" :")
            .append(isAway ?
                    "Your client is marked as away" :
                    "Your client is no longer marked as away");

        CNumericMessage msg;
        msg.Parse(line);
        return msg;
    }

    void UpdateAwayStatus() {
        bool allClientsAway = AllClientsAway();
        bool serverAway = GetNetwork()->IsIRCAway();

        if (!allClientsAway && serverAway) {
            // Mark ourselves as not away
            PutStatus("Marking us as not away since at least one client is not away");
            PutIRC(CreateAwayRequest(false));
        } else if (allClientsAway && !serverAway) {
            // Mark ourselves as away
            PutStatus("Marking us as away since all connected clients are away");
            PutIRC(CreateAwayRequest(true));
        }
    }

    void OnClientLogin() {
        CClient* client = GetClient();
        if (client != nullptr) {
            client->SetAway(false);
        }
        UpdateAwayStatus();
    }

    void OnClientDisconnect() {
        CClient* client = GetClient();
        if (client != nullptr) {
            client->SetAway(true);
        }
        UpdateAwayStatus();
    }

    EModRet OnUserRawMessage(CMessage& msg) {
        // We don't care about non AWAY messages
        if (msg.GetType() != CMessage::Type::Away) {
            return CONTINUE;
        }
        // If we sent params we are marking ourselves as away
        bool isAway = msg.GetParams().size();

        CClient* client = GetClient();
        if (client == nullptr) {
            return HALTCORE;
        }

        // Here we act as the real IRC server would and reply to the away
        // request which makes the client think it is marked as away
        client->SetAway(isAway);
        PutUser(CreateAwayResponse(isAway).ToString());
        if (isAway) {
            PutStatus("Your client is marked as away");
        } else {
            PutStatus("Your client is marked as present");
        }

        UpdateAwayStatus();
        return HALTCORE;
    }

    EModRet OnNumericMessage(CNumericMessage& msg) {
        switch (msg.GetCode()) {
            // Don't pass RPL_UNAWAY and RPL_NOWAWAY to client since we have
            // already replied in OnUserRawMessage. We do however need ZNC
            // to track if the server thinks we are away or not.
            case 305:
                GetNetwork()->SetIRCAway(false);
                return HALTCORE;
            case 306:
                GetNetwork()->SetIRCAway(true);
                return HALTCORE;
            default:
                return CONTINUE;
        }
    }
};

NETWORKMODULEDEFS(ClientAway, "Only mark as away when all clients are")
