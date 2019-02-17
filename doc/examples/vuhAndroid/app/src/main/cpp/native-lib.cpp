#include <jni.h>
#include <string>

#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include <vector>
#include "glsl2spv.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "log.h"

static char * loadFromAsset(AAssetManager* mgr,const char* file,size_t& len) {
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

static bool loadFromAsset(AAssetManager* mgr,const char* file,std::vector<char>& buf) {
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

static bool loadSaxpyComp(AAssetManager* mgr,std::vector<char>& code) {
    size_t len = 0;
    char * saxpy = loadFromAsset(mgr,"saxpy.comp",len);
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


extern "C" JNIEXPORT jstring JNICALL
Java_com_mobibrw_vuhandroid_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */,
        jobject assetManager) {
    std::string hello = "Hello from C++";
    AAssetManager * assetMgr = AAssetManager_fromJava(env, assetManager);
    saxpy(assetMgr, true);
    saxpy(assetMgr, false);
    return env->NewStringUTF(hello.c_str());
}
