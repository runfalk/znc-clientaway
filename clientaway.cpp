#include <string>
#include <sstream>

#include <znc/Client.h>
#include <znc/IRCNetwork.h>
#include <znc/main.h>
#include <znc/Modules.h>

#if (VERSION_MAJOR < 1) || (VERSION_MAJOR == 1 && VERSION_MINOR < 7)
#error The clientaway module requires ZNC version 1.7.0 or later.
#endif

struct AwaySummary {
    bool IsAway() {
        return numAway == numClients;
    }

    uint16_t numAway;
    uint16_t numClients;
};

class ClientAway : public CModule {
public:
    MODCONSTRUCTOR(ClientAway) {}

    AwaySummary GetAwaySummary() {
        uint16_t numAway = 0;
        uint16_t numClients = 0;

        for (CClient* client : GetNetwork()->GetClients()) {
            numClients++;
            if (client->IsAway()) {
                numAway++;
            }
        }
        return {numAway, numClients};
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

    CMessage CreateAwayResponse(bool isAway, AwaySummary awaySummary) {
        std::stringstream line;
        line << ":irc.znc.in " << (isAway ? 306 : 305)
             << " "
             << GetClient()->GetNick()
             << " :"
             << "Your client is " << (isAway ? "away" : "back")
             << " - "
             << awaySummary.numAway
             << "/"
             << awaySummary.numClients
             << " clients away";

        CNumericMessage msg;
        msg.Parse(line.str());
        return msg;
    }

    void UpdateAwayStatus(AwaySummary awaySummary) {
        bool allClientsAway = awaySummary.IsAway();
        bool serverAway = GetNetwork()->IsIRCAway();

        if (!allClientsAway && serverAway) {
            // Mark ourselves as not away
            PutIRC(CreateAwayRequest(false));
        } else if (allClientsAway && !serverAway) {
            // Mark ourselves as away
            PutIRC(CreateAwayRequest(true));
        }
    }

    void OnClientLogin() {
        CClient* client = GetClient();
        if (client != nullptr) {
            client->SetAway(false);
        }
        UpdateAwayStatus(GetAwaySummary());
    }

    void OnClientDisconnect() {
        CClient* client = GetClient();
        if (client != nullptr) {
            client->SetAway(true);
        }
        UpdateAwayStatus(GetAwaySummary());
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
        AwaySummary awaySummary = GetAwaySummary();
        PutUser(CreateAwayResponse(isAway, awaySummary).ToString());

        UpdateAwayStatus(awaySummary);
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
