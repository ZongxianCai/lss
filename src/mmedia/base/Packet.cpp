#include "Packet.h"

using namespace lss::mm;

// 实现 Packet 类的静态方法 NewPacket，创建一个新的 Packet 对象，并返回智能指针
PacketPtr Packet::NewPacket(int32_t size)
{
    // 计算需要分配的内存块大小：数据部分大小 + Packet 对象的大小
    auto block_size = size + sizeof(Packet);

    // 动态分配内存，分配大小为 block_size 的连续内存块，并将该内存块强制转换为 Packet* 类型
    Packet *packet = (Packet *) new char[block_size];

    // 将分配的内存块初始化为全 0
    memset((void*)packet, 0x00, block_size);

    // 初始化包的索引值为 -1，表示无效索引
    packet->index_ = -1;

    // 初始化包的类型为未知类型
    packet->type_ = kPacketTypeUnknowed;

    // 初始化包的容量为传入的 size
    packet->capacity_ = size;

    // 将扩展数据指针重置为空
    packet->ext_.reset();

    // 返回智能指针 PacketPtr，第二个参数为自定义删除器，用于释放分配的内存
    return PacketPtr(packet, [](Packet *p){
        delete [](char*)p;  // 使用 delete[] 释放 char 数组内存
    });
}