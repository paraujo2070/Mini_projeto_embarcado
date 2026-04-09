#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/logging.h>
#include <aidl/com/android/temp_froyo/BnTemp.h>
#include "TempHAL.h"

using aidl::com::android::temp_froyo::BnTemp;
using ndk::ScopedAStatus;

class TempService : public BnTemp {
private:
    TempHAL* temp_hal;
public:
    TempService() {
        temp_hal = new TempHAL();
        if (temp_hal->init()) {
            LOG(INFO) << "HAL Clima froyo inicializada com calibração.";
        } else {
            LOG(ERROR) << "Falha ao inicializar HAL Clima froyo.";
        }
    }
    ~TempService() { delete temp_hal; }

    ScopedAStatus getTemperature(float* _aidl_return) override {
        *_aidl_return = temp_hal->readTemperature();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getHumidity(float* _aidl_return) override {
        *_aidl_return = temp_hal->readHumidity();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getAmbientTemperature(float factor, float* _aidl_return) override {
        *_aidl_return = temp_hal->readAmbientTemperature(factor);
        return ScopedAStatus::ok();
    }
};

int main() {
    android::base::InitLogging(nullptr, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Iniciando daemon temp_froyo (Compensação Térmica)...";

    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    std::shared_ptr<TempService> service = ndk::SharedRefBase::make<TempService>();
    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "temp_froyo");
    
    if (status != STATUS_OK) {
        LOG(ERROR) << "Falha ao registrar serviço temp_froyo!";
        return -1;
    }

    LOG(INFO) << "Serviço temp_froyo registrado.";
    ABinderProcess_joinThreadPool();
    return 0;
}
