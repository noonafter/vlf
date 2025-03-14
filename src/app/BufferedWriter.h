//
// Created by noisemonitor on 2025/1/6.
//

#ifndef VLF_BUFFEREDWRITER_H
#define VLF_BUFFEREDWRITER_H

#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include <QDebug>
#include <QDir>

// 固定buf大小的writer，为了实现高效写入，当某一次写入数据会超出capacity时，自动丢掉溢出数据
class BufferedWriter {
public:
    // 构造函数，不需要文件路径，初始化缓冲区
    BufferedWriter(int bufferSize)
            : m_bufferSize(bufferSize) {
        m_buffer.reserve(bufferSize);  // 预分配缓冲区内存
    }

    BufferedWriter(const QString& fileName, int bufferSize) : m_file(fileName), m_bufferSize(bufferSize){
        m_buffer.reserve(bufferSize);
    }

    ~BufferedWriter() {
        // 确保缓冲区中的数据在销毁前写入文件
        flushBuffer();
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

    // 设置文件路径，如果文件夹不存在则创建
    void setDir(const QString& dirPath){
        m_dir = QDir(dirPath); // 移动赋值
        if(!m_dir.exists()){
            qDebug() << "dir not exist: " << dirPath;
            if (m_dir.mkpath(dirPath)) {
                qDebug() << "directories make success";
            } else {
                qDebug() << "directories make fail";
                return;
            }
        }
    }

    // 设置文件名并打开文件，如果文件名没改变且已经打开则直接返回
    // 注意输入的文件名不包括路径
    void setFile(const QString& fileName) {
        QString fullPath;
        if(m_dir.exists()){
            fullPath = m_dir.absoluteFilePath(fileName);
        } else {
            qWarning() << "dir not exists";
        }

        // 如果文件名没变，且文件已经打开，则不需要改名字，也不需要打开文件
        if (fullPath == m_file.fileName() && m_file.isOpen()) {
            return;
        }

        if (m_file.isOpen()) {
            flushBuffer(); // 文件名改变了，前一个文件可能还有数据在buffer中
            m_file.close();  // 保证文件关闭
        }

        m_file.setFileName(fullPath);

        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qDebug() << "Failed to open file:" << fileName;
        }

        // 绑定 QDataStream 用于不同类型的数据写入
        m_dataStream.setDevice(&m_file);
    }

    // 写入 QByteArray 数据
    void write(const QByteArray& data) {
        if (!m_file.isOpen()) {
            qDebug() << "File not opened. Please set the file name first.";
            return;
        }

        int spaceLeft = m_bufferSize - m_buffer.size();
        if(data.size() > spaceLeft){
            m_buffer.append(data.mid(0,spaceLeft));
            qWarning() << "Append data while truncate " << data.size() - spaceLeft << "bytes";
        }else{
            m_buffer.append(data);  // 将数据追加到缓存
        }

        // 如果缓冲区已满，写入文件
        if (m_buffer.size() >= m_bufferSize) {
            flushBuffer();
        }
    }

    // 写入 QByteArray 数据
    void write(const char*s, int len) {
        if (!m_file.isOpen()) {
            qDebug() << "File not opened. Please set the file name first.";
            return;
        }

        int spaceLeft = m_bufferSize - m_buffer.size();
        if(len > spaceLeft){
            m_buffer.append(s,spaceLeft);
            qDebug() << "Append data while truncate " << len - spaceLeft << "bytes";
        }else{
            m_buffer.append(s,len);  // 将数据追加到缓存
        }
        // 如果缓冲区已满，写入文件
        if (m_buffer.size() >= m_bufferSize) {
            flushBuffer();
        }
    }

    QString fileName(){
        return m_file.fileName();
    }

    // 写入 int32 数据，通过datastream如果保证buf大小一定呢？是否也需要<<前检查？
    void writeInt32(int32_t value) {
        if (!m_file.isOpen()) {
            qDebug() << "File not opened. Please set the file name first.";
            return;
        }

        m_dataStream << value;  // 使用 QDataStream 写入 int32 数据
    }

    // 写入 float 数据
    // 这里可能还有改一下datastream的精度
    void writeFloat(float value) {
        if (!m_file.isOpen()) {
            qDebug() << "File not opened. Please set the file name first.";
            return;
        }

        m_dataStream << value;  // 使用 QDataStream 写入 float 数据
    }

private:
    // 刷新缓冲区
    void flushBuffer() {
        if (!m_buffer.isEmpty()) {
            m_file.write(m_buffer);  // 写入文件
            m_buffer.resize(0);       // 清空缓存区，减少内存占用
        }
    }

private:
    QFile m_file;
    QDir m_dir;
    QByteArray m_buffer;  // 缓存区
    QDataStream m_dataStream;  // 用于写入不同类型的数据
    int m_bufferSize;     // 缓冲区大小
};

//// 使用示例
//int main() {
//    BufferedWriter writer(1024);  // 设置缓存大小为1KB
//
//    // 设置文件路径并写入数据
//    writer.setFileName("file1.dat");
//
//    // 写入一些字节数据
//    writer.write("Writing some binary data.\n");
//
//    // 写入 int32 和 float 数据
//    writer.writeInt32(123456789);  // 写入 int32
//    writer.writeFloat(3.14159f);    // 写入 float
//
//    // 再次写入一些字节数据
//    writer.write("More data.\n");
//
//    return 0;
//}


#endif //VLF_BUFFEREDWRITER_H
