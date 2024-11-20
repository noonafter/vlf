//
// Created by noisemonitor on 2024/11/20.
//

#ifndef VLF_UDP_WAVE_CONFIG_H
#define VLF_UDP_WAVE_CONFIG_H

// 作为udp和波形参数的配置管理类，独立进行json配置文件的读写，并可以作为参数在不同模块间传递，将界面类和线程工作类进行解耦
// 可以完全对应json配置文件的结构进行设计，方便进行添加和修改
// udp和波形参数作为独立类，可以在内部都参数读写进行控制：
// 例如，如果存在多线程的问题，可以在内部加锁进行读写控制，对外提供线程安全的读写功能，降低外部使用的复杂性。这里只有界面类在的主线程会进行修改，也可以不加
class udp_wave_config {



};


#endif //VLF_UDP_WAVE_CONFIG_H
