/*
    dash.c - Dashboard
 */
#include "esp.h"

#if 0
{
    ports: [
        { rxBytes: NN, rxPackets: NN, txBytes: NN, txPackets: NN }
    ],
    vlans: [
        { rxBytes: NN, rxPackets: NN, txBytes: NN, txPackets: NN }
    ],
    system: {
        rxBytes: NN, rxPackets: NN, txBytes: NN, txPackets: NN,
        ports: {
            online: NN,
            offline: NN,
        },
        vlans: {
            online: NN,
            offline: NN,
        }
        events: {
            total: NN,
        }
    }
}
#endif

#if DEMO || 1
    //  Separate table
static void demoData() {
    EdiGrid     *ports;
    EdiRec      *port;
    cchar       *value;
    int         r;

    //  Separate table
    ports = readTable("port");
    if (ports) {
        for (r = 0; r < ports->nrecords; r++) {
            r += (random() % (ports->nrecords / 3));
            if (r < ports->nrecords) {
                port = ports->records[r];
                value = ediGetFieldValue(port, "rxBytes");
                ediSetField(port, "rxBytes", itos(stoi(value) + (random() % 1000)));
                value = ediGetFieldValue(port, "rxPackets");
                ediSetField(port, "rxPackets", itos(stoi(value) + (random() % 1000)));
                updateRec(port);
            }
        }
    }
}
#endif

static cchar *getDashData(HttpConn *conn) {
    EdiGrid     *events, *ports, *vlans;
    MprBuf      *buf;
    int         nevents;

    espSetConn(conn);
    buf = mprCreateBuf(0, 0);
    mprPutToBuf(buf, "{");
    ports = ediFilterGridFields(readTable("port"), "rxBytes,rxPackets,txBytes,txPackets,mode", 1);
    mprPutToBuf(buf, "\"ports\": %s,", ediGridAsJson(ports, 0));
    vlans = ediFilterGridFields(readTable("vlan"), "rxBytes,rxPackets,txBytes,txPackets,mode", 1);
    mprPutToBuf(buf, "\"vlans\": %s,", ediGridAsJson(vlans, 0));
    // system = ediFilterRecFields(readRec("system", "1"), "rxBytes,rxPackets,txBytes,txPackets", 1);

    nevents = 0;
    if ((events = readTable("event")) != 0) {
        nevents = events->nrecords;
        events->nrecords = min(events->nrecords, 5);
    }
    mprPutToBuf(buf, "\"system\":{\"events\": %s, \"eventCount\": %d}", ediGridAsJson(events, 0), nevents);

    mprPutToBuf(buf, "}");

#if DEMO || 1
    demoData();
#endif
    return mprGetBufStart(buf);
}

static void getDash() {
    render(getDashData(getConn()));
}

static void updateStream(HttpConn *conn) {
    Esp     *esp;
    ssize   count;

    esp = MPR->espService;
    if (HTTP_STATE_PARSED <= conn->state && conn->state <= HTTP_STATE_CONTENT) {
        if (esp->reloading) {
            httpSendClose(conn, WS_STATUS_OK, "OK");
            return;
        }
        if ((count = httpSendBlock(conn, WS_MSG_TEXT, getDashData(conn), -1, 0)) < 0) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot send big message");
        }
    }
}

static void getSocketStream() {
    HttpConn    *conn;
    MprEvent    *timer;
    MprTicks    period;

    conn = getConn();
    dontAutoFinalize();
    updateStream(conn);
    period = stoi(espGetConfig(conn->rx->route, "settings.refresh", "1000"));
    timer = mprCreateTimerEvent(conn->dispatcher, "dashboard", period, updateStream, conn, 0);
    httpSetWebSocketData(conn, timer);
}

ESP_EXPORT int esp_controller_kickstart_dash(HttpRoute *route)
{
    espDefineAction(route, "dash/get", getDash);
    espDefineAction(route, "dash/stream", getSocketStream);
    return 0;
}
