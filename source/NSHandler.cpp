#include <switch.h>
class NSHandler{
private:
    void getApplications(){
        NsApplicationRecord records[24];
        s32 recordsNum;
        nsListApplicationRecord(reinterpret_cast<NsApplicationRecord*>(&records),
            sizeof(records) / sizeof(NsApplicationRecord*), 0, &recordsNum);
    }
};