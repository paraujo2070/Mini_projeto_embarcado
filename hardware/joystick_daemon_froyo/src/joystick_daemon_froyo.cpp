#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/logging.h>
#include <aidl/com/android/joystick_froyo/BnJoystick.h>
#include "JoystickHAL.h"

using aidl::com::android::joystick_froyo::BnJoystick;
using ndk::ScopedAStatus;

class JoystickService : public BnJoystick
{
private:
    JoystickHAL *joystick;

public:
    JoystickService()
    {
        joystick = new JoystickHAL();
        if (joystick->init())
        {
            LOG(INFO) << "HAL do Joystick froyo inicializada com sucesso.";
        }
        else
        {
            LOG(ERROR) << "Falha ao inicializar HAL do Joystick froyo.";
        }
    }
    ~JoystickService()
    {
        delete joystick;
    }
    ScopedAStatus getJoystickState(int32_t *_aidl_return) override
    {
        if (joystick)
        {
            int val = joystick->readJoystick();
            *_aidl_return = val;

            // Log formatado
            std::string dir;
            switch (val)
            {
            case 0x01:
                dir = "UP";
                break;
            case 0x02:
                dir = "LEFT";
                break;
            case 0x04:
                dir = "DOWN";
                break;
            case 0x08:
                dir = "PRESS";
                break;
            case 0x10:
                dir = "RIGHT";
                break;
            case 0x00:
                dir = "NONE";
                break;
            default:
                dir = "UNKNOWN";
                break;
            }
            LOG(INFO) << "Joystick: " << dir << " (0x" << std::hex << val << ")";
        }
        return ScopedAStatus::ok();
    }
};

int main()
{
    android::base::InitLogging(nullptr, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "Iniciando daemon joystick_froyo...";

    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    std::shared_ptr<JoystickService> service = ndk::SharedRefBase::make<JoystickService>();
    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "joystick_froyo");

    if (status != STATUS_OK)
    {
        LOG(ERROR) << "Falha ao registrar serviço joystick_froyo!";
        return -1;
    }

    LOG(INFO) << "Serviço joystick_froyo registrado.";
    ABinderProcess_joinThreadPool();
    return 0;
}
