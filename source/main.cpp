#define TESLA_INIT_IMPL
#include "netload.hpp"
#include <tesla.hpp>
#include <format>

constexpr const SocketInitConfig sockConf = {
    .tcp_tx_buf_size = 64000,
    .tcp_rx_buf_size = 64000,
    .tcp_tx_buf_max_size = 0x25000,
    .tcp_rx_buf_max_size = 0x25000,

    .udp_tx_buf_size = 8192,
    .udp_rx_buf_size = 8192,

    .sb_efficiency = 1,

    .num_bsd_sessions = 0,
    .bsd_service_type = BsdServiceType_Auto,
};

Thread thread;
Result rc = 0;

std::string statusMessage = "Initialisation...";
std::string errorMessage = "";
int progressPercent = 0;

class GuiNetloader : public tsl::Gui {
private:

public:
    GuiNetloader() { }
    ~GuiNetloader() { }

    tsl::elm::Element* createUI() override {
        auto *rootFrame = new tsl::elm::OverlayFrame("Netloader", "v1.0.0");
        rootFrame->setContent(
            new tsl::elm::CustomDrawer(
                [this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                    renderer->drawString((errorMessage.empty() ? statusMessage : errorMessage).c_str(), false, 70, 350, 20, tsl::style::color::ColorText);

                    // Progress bar
                    if (progressPercent > 0) {
                        renderer->drawRect(65, 400, 300, 50, renderer->a(0x1780d3));
                        renderer->drawRect(65, 400, 300 * progressPercent / 100, 50, renderer->a(0xF0F0));
                    }
                }
            )
        );

        return rootFrame;
    }

    void update() {
        static int counter = 0;
        if ((counter++ % 5) != 0)
            return;

        netloader::State state = {};
        netloader::getState(&state);

        u32 ip;
        nifmGetCurrentIpAddress(&ip);

        if (!state.errormsg.empty())
            errorMessage = state.errormsg;
        else if (ip == 0) {
            statusMessage = "No internet connection!";
        } else if (!state.sock_connected) {
            statusMessage = std::format("Waiting for nxlink to connect…\nIP: {}.{}.{}.{}, Port: {}", ip&0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF, NXLINK_SERVER_PORT);
        }else {
            statusMessage = std::format("Transferring…\n{} out of {} KiB written", state.filetotal/1024, state.filelen/1024);
            progressPercent = (state.filetotal * 100) / state.filelen;
        }

        if (state.launch_app && !state.activated) {
            netloader::setNext();
            tsl::Overlay::get()->close();
        }
    }


};

class OverlayNetloader : public tsl::Overlay {
public:
    OverlayNetloader() { }
    ~OverlayNetloader() { }

    void initServices() {
        fsdevMountSdmc();
        tsl::hlp::doWithSmSession([this] {
            if (R_SUCCEEDED(rc = socketInitialize(&sockConf))){
                rc = nifmInitialize(NifmServiceType_User);
                if (R_SUCCEEDED(rc)){
                    nxlinkStdio();
                } else {
                    errorMessage = "Error on NIFM initialisation: " + std::to_string(rc);
                }
            }else {
                errorMessage = "Error on socket initialisation: " + std::to_string(rc);
            }
        });
        threadCreate(&thread, netloader::task, nullptr, nullptr, 0x1000, 0x2C, -2);
        threadStart(&thread);
    }
    void exitServices() {
        netloader::signalExit();

        threadWaitForExit(&thread);
        threadClose(&thread);

        fsdevUnmountAll();
        nifmExit();
        socketExit();
    }

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiNetloader>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayNetloader>(argc, argv);
}
