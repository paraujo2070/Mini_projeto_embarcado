#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_ibinder.h>
#include <android-base/logging.h>
#include <aidl/com/android/temp_froyo/BnTemp.h>
#include <aidl/com/android/temp_froyo/ITempListener.h>
#include "TempHAL.h"
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>

using aidl::com::android::temp_froyo::BnTemp;
using aidl::com::android::temp_froyo::ITempListener;
using ndk::ScopedAStatus;

class TempService : public BnTemp {
private:
    TempHAL* temp_hal;
    
    struct Client {
        std::shared_ptr<ITempListener> listener;
        float factor;
        pid_t pid;
    };
    
    std::vector<Client> clients;
    std::mutex clients_mutex;
    std::thread worker;
    bool worker_running;
    int telemetry_counter = 0;

    void pollingLoop() {
        LOG(INFO) << "[SERVICE] Iniciando thread de polling do clima (HTS221).";
        while (worker_running) {
            float t = 0, h = 0;
            
            if (temp_hal->readClimate(t, h)) {
                std::lock_guard<std::mutex> lock(clients_mutex);
                
                // Telemetria Periódica: Loga o estado do sistema a cada 10 segundos
                if (++telemetry_counter >= 10) {
                    LOG(INFO) << "[TELEMETRIA] T: " << t << "°C | H: " << h << "% | Clients: " << clients.size();
                    telemetry_counter = 0;
                }

                for (auto it = clients.begin(); it != clients.end(); ) {
                    float t_amb = temp_hal->readAmbientTemperature(it->factor);
                    
                    // Notifica o callback unificado
                    auto status = it->listener->onClimateUpdate(t, h, t_amb);
                    if (!status.isOk()) {
                        LOG(WARNING) << "[CLIENT] Listener PID " << it->pid << " falhou/desconectou. Removendo.";
                        it = clients.erase(it);
                    } else {
                        // Notifica os callbacks individuais para compatibilidade
                        it->listener->onTemperatureUpdate(t);
                        it->listener->onHumidityUpdate(h);
                        it->listener->onAmbientTemperatureUpdate(t_amb);
                        ++it;
                    }
                }
            } else {
                LOG(ERROR) << "[HARDWARE] Falha crítica de leitura no barramento I2C (HTS221)!";
                std::this_thread::sleep_for(std::chrono::seconds(2)); // Espera mais em caso de erro
                continue;
            }
            
            // Verificação de encerramento da thread
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                if (clients.empty()) {
                    LOG(INFO) << "[SERVICE] Nenhum cliente ativo. Entrando em modo repouso.";
                    worker_running = false;
                    break;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

public:
    TempService() {
        temp_hal = new TempHAL();
        worker_running = false;
        if (temp_hal->init()) {
            LOG(INFO) << "[INIT] HAL Clima froyo inicializada com sucesso.";
        } else {
            LOG(ERROR) << "[INIT] FALHA ao inicializar HAL Clima froyo!";
        }
    }
    
    ~TempService() {
        worker_running = false;
        if (worker.joinable()) worker.join();
        delete temp_hal;
    }

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

    ScopedAStatus registerListener(const std::shared_ptr<ITempListener>& listener, float factor) override {
        pid_t calling_pid = AIBinder_getCallingPid();
        std::lock_guard<std::mutex> lock(clients_mutex);
        
        // Evita duplicidade de registro usando o Binder subjacente
        auto it = std::find_if(clients.begin(), clients.end(), [&](const Client& c) {
            return c.listener->asBinder().get() == listener->asBinder().get();
        });
        
        if (it == clients.end()) {
            clients.push_back({listener, factor, calling_pid});
            LOG(INFO) << "[BINDER] Novo registro de listener. PID: " << calling_pid 
                      << " | Fator: " << factor << " | Total: " << clients.size();
        }

        if (!worker_running) {
            if (worker.joinable()) worker.join();
            worker_running = true;
            worker = std::thread(&TempService::pollingLoop, this);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus unregisterListener(const std::shared_ptr<ITempListener>& listener) override {
        pid_t calling_pid = AIBinder_getCallingPid();
        std::lock_guard<std::mutex> lock(clients_mutex);
        
        clients.erase(std::remove_if(clients.begin(), clients.end(), [&](const Client& c) {
            return c.listener->asBinder().get() == listener->asBinder().get();
        }), clients.end());
        
        LOG(INFO) << "[BINDER] Listener removido por PID: " << calling_pid 
                  << ". Restantes: " << clients.size();
        return ScopedAStatus::ok();
    }
};

int main() {
    android::base::InitLogging(nullptr, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "================================================";
    LOG(INFO) << "Iniciando Daemon de Clima Reativo (froyo)";
    LOG(INFO) << "================================================";

    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_startThreadPool();

    std::shared_ptr<TempService> service = ndk::SharedRefBase::make<TempService>();
    binder_status_t status = AServiceManager_addService(service->asBinder().get(), "temp_froyo");
    
    if (status != STATUS_OK) {
        LOG(ERROR) << "ERRO FATAL: Falha ao registrar serviço temp_froyo!";
        return -1;
    }

    LOG(INFO) << "Serviço temp_froyo registrado com sucesso.";
    ABinderProcess_joinThreadPool();
    return 0;
}
