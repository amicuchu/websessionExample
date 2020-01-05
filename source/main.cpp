#include <switch.h>
#include <memory>
#include "WebSession.hpp"
#include "Log.hpp"
#include "ExampleHandler.hpp"

extern "C"{
    #include <memory.h>
    static Result _webTLVWrite(WebCommonTLVStorage *storage, u16 type, const void* argdata, u16 argdata_size, u16 argdata_size_total) {
        Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        size_t i, count, offset;
        u8 *dataptr = storage->data;
        WebArgHeader *hdr = (WebArgHeader*)dataptr;
        WebArgTLV *tlv;
        size_t size = sizeof(storage->data);

        offset = sizeof(WebArgHeader);
        if (size < offset) return rc;
        if (argdata_size > argdata_size_total) argdata_size = argdata_size_total;

        count = hdr->total_entries;
        tlv = (WebArgTLV*)&dataptr[offset];

        for (i=0; i<count; i++) {
            if (size < offset + sizeof(WebArgTLV)) return rc;

            tlv = (WebArgTLV*)&dataptr[offset];

            if (tlv->type == type) {
                if (tlv->size != argdata_size_total) return rc;
                break;
            }

            offset+= sizeof(WebArgTLV) + tlv->size;
            if (size < offset) return rc;
        }

        if (size < offset + sizeof(WebArgTLV) + argdata_size_total) return rc;

        tlv = (WebArgTLV*)&dataptr[offset];

        if (tlv->type != type) {
            if (hdr->total_entries == 0xFFFF) return rc;

            tlv->type = type;
            tlv->size = argdata_size_total;
            hdr->total_entries++;
        }

        offset+= sizeof(WebArgTLV);
        memcpy(&dataptr[offset], argdata, argdata_size);
        if (argdata_size_total != argdata_size) memset(&dataptr[offset+argdata_size], 0, argdata_size_total-argdata_size);

        return 0;
    }
}

Logger g_Logger{};

int main(int argc, char* argv[])
{
    Result rc=0;

    // This example uses a text console, as a simple way to output text to the screen.
    // If you want to write a software-rendered graphics application,
    //   take a look at the graphics/simplegfx example, which uses the libnx Framebuffer API instead.
    // If on the other hand you want to write an OpenGL based application,
    //   take a look at the graphics/opengl set of examples, which uses EGL instead.
    consoleInit(NULL);

    //consoleUpdate(NULL);

    g_Logger.print("Test");

    WebCommonConfig config;
    WebCommonReply reply;
    WebExitReason exitReason;

    webPageCreate(&config, "https://camitest.my.to/");
    g_Logger.print("Web config created");
    WebSession webSession(&config.holder);

    u32 dummyValA = 0;
    webConfigSetJsExtension(&config, 1);
    rc = _webTLVWrite(&config.arg, WebArgType_SessionBootMode, &dummyValA, sizeof(u32), sizeof(u32));
    u8 dummyVal = 1;
    rc = _webTLVWrite(&config.arg, WebArgType_SessionFlag, &dummyVal, sizeof(u8), sizeof(u8));
    if (R_FAILED(rc)) {
        return 1;
    }

    ExampleHandler exhand{};

    rc = webConfigSetWhitelist(&config, "^http*");
    if (R_SUCCEEDED(rc)) {
        g_Logger.print("TLV configured");

        webSession.addHandler(std::unique_ptr<WebSessionHandler>(&exhand));

        webSession.startThread();
        consoleUpdate(NULL);
        rc = webConfigShow(&config, &reply);
        g_Logger.print("Web invoked");
    }

    webSession.waitForThread();
    g_Logger.print("Thread exit waitted");
    
    consoleExit(NULL);
    return 0;
}