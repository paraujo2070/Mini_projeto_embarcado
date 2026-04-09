#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/logging.h>
#include <aidl/com/android/pressure_froyo/BnPressure.h>
#include "PressureHAL.h"

using aidl::com::android::pressure_froyo::BnPressure;
using ndk::ScopedAStatus;

class PressureService : public BnPressure {
private:
    PressureHAL* pressure_hal;
public:
    PressureService() {
        pressure_hal = new PressureHAL();
        if (pressure_hal->init()) {
            LOG(INFO) << "HAL de Pressão froyo inicializada.";
        } else {
            LOG(ERROR) << "Falha ao inicializar HAL de Pressão froyo.";
        }
    }
    ~PressureService() { delete pressure_hal; }

    ScopedAStatus getPressure(float* _aidl_return) override {
        if (pressure_hal) {
            *_aidl_return = pressure_hal->readPressure();
        } else {
            *_aidl_return = -1.0f;
        }
        return ScopedAStatus::ok();
    }
};

int main() {
    android::base::InitLogging(nullptr, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Iniciando daemon pressure_froyo...";

    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    std::shared_ptr<PressureService> service = ndk::SharedRefBase::make<PressureService>();
    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "pressure_froyo");
    
    if (status != STATUS_OK) {
        LOG(ERROR) << "Falha ao registrar serviço pressure_froyo!";
        return -1;
    }

    LOG(INFO) << "Serviço pressure_froyo registrado.";
    ABinderProcess_joinThreadPool();
    return 0;
}
