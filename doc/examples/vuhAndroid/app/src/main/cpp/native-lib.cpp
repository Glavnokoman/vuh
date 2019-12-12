#include <jni.h>
#include <string>

#include <vuh/array.hpp>
#include <vuh/image.hpp>
#include <vuh/vuh.h>
#include <vector>
#include "glsl2spv.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>

static char * loadFromAsset(AAssetManager* mgr, const char* file, size_t& len) {
    AAsset * asset = AAssetManager_open(mgr, file, AASSET_MODE_BUFFER);
    if (NULL != asset) {
        len = AAsset_getLength(asset);
        char *content = new char[len];
        AAsset_read(asset, content, len);
        AAsset_close(asset);
        return content;
    }
    return NULL;
}

static bool loadFromAsset(AAssetManager* mgr, const char* file, std::vector<char>& buf) {
    AAsset * asset = AAssetManager_open(mgr, file, AASSET_MODE_BUFFER);
    if(NULL != asset ) {
        size_t length = AAsset_getLength(asset);
        buf.resize(length);
        AAsset_read(asset, &buf[0], length);
        AAsset_close(asset);
        return true;
    }
    return false;
}

static bool loadComp(AAssetManager* mgr, const char* file, std::vector<char>& code) {
    size_t len = 0;
    char * saxpy = loadFromAsset(mgr, file, len);
    if(NULL != saxpy) {
        std::vector<unsigned int> spirv;
        bool suc = glsl2spv(VK_SHADER_STAGE_COMPUTE_BIT,saxpy,spirv);
        delete saxpy;
        if (suc && (spirv.size() > 0)){
            const int len = spirv.size() * sizeof(unsigned int);
            code.resize(len);
            memmove(&code[0],&spirv[0],len);
            return suc;
        }
    }
    return false;

}

static bool loadSaxpyComp(AAssetManager* mgr,std::vector<char>& code) {
    return loadComp(mgr, "saxpy.comp", code);
}

static bool loadSaxpySpv(AAssetManager* mgr,std::vector<char>& code) {
    return loadFromAsset(mgr,"saxpy.spv",code);
}

static auto saxpy(AAssetManager* mgr,bool comp)-> bool {
    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    auto instance = vuh::Instance();
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device

        auto d_y = vuh::Array<float>(device,
                                     y); // allocate memory on device and copy data from host
        auto d_x = vuh::Array<float>(device, x); // same for x

        using Specs = vuh::typelist<uint32_t>;
        struct Params {
            uint32_t size;
            float a;
        };
        LOGD("saxpy before %f",y[0]);
        std::vector<char> code;
        bool suc = false;
        if (comp) {
            suc = loadSaxpyComp(mgr, code);
        } else {
            suc = loadSaxpySpv(mgr, code);
        }
        if (suc) {
            auto program = vuh::Program<Specs, Params>(device,
                                                       code); // define the kernel by linking interface and spir-v implementation
            program.grid(128 / 64).spec(64)({128, a}, d_y, d_x); // run once, wait for completion
            d_y.toHost(begin(y));                              // copy data back to host
            LOGD("saxpy after %f",y[0]);
        }

        return suc;
    }
    return false;
}

/*example for Validation Layers*/

/// Default debug reporter used when user did not care to provide his own.
static auto VKAPI_ATTR debugReporter(
        VkDebugReportFlagsEXT       msgFlags,
        VkDebugReportObjectTypeEXT  ,
        uint64_t                    ,
        size_t                      ,
        int32_t                     msgCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       /*pUserData*/
)-> VkBool32
{
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        LOGE("ERROR: [%s] Code %i : %s", pLayerPrefix, msgCode, pMessage);
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        LOGW("WARNING: [%s] Code %i : %s", pLayerPrefix, msgCode, pMessage);
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        LOGW("PERFORMANCE WARNING: [%s] Code %i : %s", pLayerPrefix, msgCode, pMessage);
    } else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        LOGI("INFO: [%s] Code %i : %s", pLayerPrefix, msgCode, pMessage);
    } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        LOGD("DEBUG: [%s] Code %i : %s", pLayerPrefix, msgCode, pMessage);
    }

    // Returning false tells the layer not to stop when the event occurs, so
    // they see the same behavior with and without validation layers enabled.
    return VK_FALSE;
}

static auto saxpyValidationLayers(AAssetManager* mgr)-> bool {
    // On Android we need to explicitly select all layers
    // Keep this order ,do'nt change this
    std::vector<const char*> layer_names;
    layer_names.push_back("VK_LAYER_GOOGLE_threading");
    layer_names.push_back("VK_LAYER_LUNARG_parameter_validation");
    layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
    layer_names.push_back("VK_LAYER_LUNARG_core_validation");
    layer_names.push_back("VK_LAYER_LUNARG_swapchain");
    layer_names.push_back("VK_LAYER_GOOGLE_unique_objects");

    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    vuh::debug_reporter_flags_t flags = DEF_DBG_REPORT_FLAGS | VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    auto instance = vuh::Instance(layer_names, {VK_EXT_DEBUG_REPORT_EXTENSION_NAME}, {nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}, debugReporter, flags);
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device

        auto d_y = vuh::Array<float>(device,
                                     y); // allocate memory on device and copy data from host
        auto d_x = vuh::Array<float>(device, x); // same for x

        using Specs = vuh::typelist<uint32_t>;
        struct Params {
            uint32_t size;
            float a;
        };
        LOGD("saxpy debug before %f",y[0]);
        std::vector<char> code;
        bool suc = loadSaxpySpv(mgr, code);
        if (suc) {
            auto program = vuh::Program<Specs, Params>(device,
                                                       code); // define the kernel by linking interface and spir-v implementation
            program.grid(128 / 64).spec(64)({128, a}, d_y, d_x); // run once, wait for completion
            d_y.toHost(begin(y));                              // copy data back to host
            LOGD("saxpy debug after %f",y[0]);
        }

        return suc;
    }
    return false;
}

static auto saxpyAsync(AAssetManager* mgr,bool comp)-> bool {
    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    auto instance = vuh::Instance();
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device

        using Specs = vuh::typelist<uint32_t>;
        struct Params {
            uint32_t size;
            float a;
        };
        std::vector<char> code;
        bool suc = false;
        if (comp) {
            suc = loadSaxpyComp(mgr, code);
        } else {
            suc = loadSaxpySpv(mgr, code);
        }
        if (suc) {
            auto program = vuh::Program<Specs, Params>(device,
                                                       code); // define the kernel by linking interface and spir-v implementation
            {
                LOGD("saxpy async before %f",y[0]);

                auto d_y = vuh::Array<float>(device,
                                             y); // allocate memory on device and copy data from host
                auto d_x = vuh::Array<float>(device, x); // same for x
                auto d_z = vuh::Array<float>(device, x); // same for x

                auto delay = program.grid(128 / 64).spec(64).run_async({128, a}, d_y, d_x);

                delay.wait(); // run once, wait for completion
                d_y.toHost(begin(y));                              // copy data back to host
                LOGD("saxpy async after %f", y[0]);
            }

            {
                /*reset args*/
                y = std::vector<float>(128, 1.0f);
                LOGD("saxpy async suspend before %f", y[0]);

                auto d_y = vuh::Array<float>(device,
                                             y); // allocate memory on device and copy data from host
                auto d_x = vuh::Array<float>(device, x); // same for x

                /*run but suspend wait for signal*/
                auto delay = program.grid(128 / 64).spec(64).run_async({128, a}, true, d_y, d_x);
                delay.resume();
                delay.wait(); // run once, wait for completion
                d_y.toHost(begin(y));                              // copy data back to host
                LOGD("saxpy async suspend after %f", y[0]);
            }

            {
                /*reset args*/
                y = std::vector<float>(128, 1.0f);
                LOGD("saxpy async unsuspend before %f", y[0]);
                auto d_y = vuh::Array<float>(device,
                                              y); // allocate memory on device and copy data from host
                auto d_x = vuh::Array<float>(device, x); // same for x

                /*run but suspend wait for signal*/
                auto delay3 = program.grid(128 / 64).spec(64).run_async({128, a}, false, d_y,
                                                                        d_x);
                delay3.wait(); // run once, wait for completion
                d_y.toHost(begin(y));
                LOGD("saxpy async unsuspend after %f", y[0]);
            }
        }

        return suc;
    }
    return false;
}

static auto saxpy_epoll(AAssetManager* mgr)-> bool {
    /*Since Linux 2.6.8,the size argument is ignored, but must be greater than zero;*/
    int efd = epoll_create1(0);
    if(-1 == efd) {
        int err = errno;
        LOGE("saxpy epoll create failed error =%d(%s)", err, strerror(err));
    }

    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    auto instance = vuh::Instance();
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device

        auto d_y = vuh::Array<float>(device,
                                     y); // allocate memory on device and copy data from host
        auto d_x = vuh::Array<float>(device, x); // same for x

        using Specs = vuh::typelist<uint32_t>;
        struct Params {
            uint32_t size;
            float a;
        };
        LOGD("saxpy epoll before %f",y[0]);
        std::vector<char> code;
        bool suc = loadSaxpySpv(mgr, code);
        if (suc) {
            auto program = vuh::Program<Specs, Params>(device,
                                                       code); // define the kernel by linking interface and spir-v implementation
            auto delay = program.grid(128 / 64).spec(64).run_async({128, a}, true, d_y, d_x); // run once, wait for completion
            LOGD("saxpy epoll supportFenceFd=%d",delay.supportFenceFd());
            if(delay.supportFenceFd() && (-1 != efd)) {
                int fd;
                auto result = delay.fenceFd(fd);
                if(vk::Result::eSuccess == result) {
                    struct epoll_event ev;
                    ev.data.fd = fd;
                    ev.events = EPOLLIN /*| EPOLLET*/ | EPOLLONESHOT;
                    int s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
                    if(-1 == s) {
                        int err = errno;
                        LOGE("saxpy epoll_ctrl failed error =%d(%s)!", err, strerror(err));
                        delay.resume();
                        delay.wait();
                    } else {
                        delay.resume();
                        struct epoll_event evs;
                        int n = epoll_wait(efd, &evs, 1, -1);
                        if((n <=0) || (evs.events & EPOLLERR) || (evs.events & EPOLLHUP) || (!(evs.events & EPOLLIN))) {
                            /* An error has occured on this fd, or the socket is not
                            ready for reading (why were we notified then?) */
                            int err = errno;
                            LOGE("saxpy epoll error =%d(%s)\n",err,strerror(err));
                            close(evs.data.fd);
                        } else if(fd == evs.data.fd) {
                            close(evs.data.fd);
                            LOGI("saxpy epoll wait success \n");
                        } else {
                            LOGE("saxpy epoll error fd incorrect\n");
                        }
                    }
                } else {
                    delay.resume();
                    delay.wait();
                }
            } else {
                delay.resume();
                delay.wait();
            }
            d_y.toHost(begin(y));                              // copy data back to host
            LOGD("saxpy epoll after %f",y[0]);
        }

        return suc;
    }

    if(-1 != efd) {
        close(efd);
    }
    return false;
}

static auto saxpy_image(AAssetManager* mgr)-> bool {
    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    auto instance = vuh::Instance();
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device

        auto d_y = vuh::img::TransArray<float>(device, y); // allocate memory on device and copy data from host
        auto d_x = vuh::img::TransArray<float>(device, x); // same for x
        auto i_y = vuh::Image2D<float>(device, d_y, 16);
        auto i_x = vuh::Image2D<float>(device, d_x, 16);
        using Specs = vuh::typelist<uint32_t>;
        struct Params {
            float a;
        };
        LOGD("saxpy_image before %f",y[0]);
        std::vector<char> code;
        bool suc = loadComp(mgr, "imgsaxpy.comp", code);
        if (suc) {
            auto program = vuh::Program<Specs, Params>(device,
                                                       code); // define the kernel by linking interface and spir-v implementation
            program.grid(128 / 64).spec(64)({a}, i_y, i_x); // run once, wait for completion
            auto v = i_y.toHost();                              // copy data back to host
            v.toHost(begin(y));
            LOGD("saxpy_image %f", y[0]);
        }

        return suc;
    }
    return false;
}

static auto saxpy_buffer_image(AAssetManager* mgr)-> bool {
    auto y = std::vector<float>(128, 1.0f);
    auto x = std::vector<float>(128, 2.0f);
    const auto a = 0.1f; // saxpy scaling constant
    auto instance = vuh::Instance();
    if (instance.devices().size() > 0) {
        auto device = instance.devices().at(0);  // just get the first compute-capable device
        bool suc = false;
        {
            auto d_y = vuh::Array<float>(device,
                                         y); // allocate memory on device and copy data from host
            auto d_x = vuh::img::TransArray<float>(device, x); // same for x

            auto i_x = vuh::CombinedImage2D<float>(device, d_x, 16);

            using Specs = vuh::typelist<uint32_t>;
            struct Params {
                uint32_t size;
                float a;
            };
            std::vector<char> code;
            suc = loadComp(mgr, "imgbufsaxpy.comp", code);
            if (suc) {
                auto program = vuh::Program<Specs, Params>(device,
                                                           code); // define the kernel by linking interface and spir-v implementation
                program.grid(128 / 64).spec(64)({128, a}, d_y,
                                                i_x); // run once, wait for completion
                auto z = std::vector<float>(128, 0.0f);
                d_y.toHost(begin(z));                              // copy data back to host
                LOGD("saxpy_buffer_image after %f,%f,%f,%f,%f,%f,%f,%f,%f,%f", z[0], z[1], z[2],
                     z[3], z[4], z[5], z[6], z[7], z[8], z[9]);
            } else {
                LOGD("saxpy_buffer_image failed");
            }
        }
        return suc;
    }
    return false;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_mobibrw_vuhandroid_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */,
        jobject assetManager) {
    std::string hello = "Hello from C++";
    AAssetManager * assetMgr = AAssetManager_fromJava(env, assetManager);
    saxpy(assetMgr, true);
    saxpy(assetMgr, false);
    saxpyValidationLayers(assetMgr);
    saxpyAsync(assetMgr, false);
    saxpy_epoll(assetMgr);
    //saxpy_image(assetMgr);
    saxpy_buffer_image(assetMgr);
    return env->NewStringUTF(hello.c_str());
}
