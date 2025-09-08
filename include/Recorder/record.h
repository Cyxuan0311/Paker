#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <memory>

namespace Recorder {

/**
 * @brief 记录安装库文件路径的类
 * 
 * 用于记录安装过程中的库文件路径，便于后续的删除、显示路径等操作
 */
class Record {
public:
    /**
     * @brief 构造函数
     * @param record_file 记录文件路径
     */
    explicit Record(const std::string& record_file = "install_record.json");
    
    /**
     * @brief 析构函数
     */
    ~Record();
    
    /**
     * @brief 添加一个库的安装记录
     * @param package_name 包名
     * @param install_path 安装路径
     * @param files 安装的文件列表
     */
    void addPackageRecord(const std::string& package_name, 
                         const std::string& install_path,
                         const std::vector<std::string>& files = {});
    
    /**
     * @brief 添加单个文件记录
     * @param package_name 包名
     * @param file_path 文件路径
     */
    void addFileRecord(const std::string& package_name, const std::string& file_path);
    
    /**
     * @brief 获取指定包的所有文件路径
     * @param package_name 包名
     * @return 文件路径列表
     */
    std::vector<std::string> getPackageFiles(const std::string& package_name) const;
    
    /**
     * @brief 获取指定包的安装路径
     * @param package_name 包名
     * @return 安装路径
     */
    std::string getPackageInstallPath(const std::string& package_name) const;
    
    /**
     * @brief 获取所有已安装的包名
     * @return 包名列表
     */
    std::vector<std::string> getAllPackages() const;
    
    /**
     * @brief 检查包是否已安装
     * @param package_name 包名
     * @return 是否已安装
     */
    bool isPackageInstalled(const std::string& package_name) const;
    
    /**
     * @brief 删除包的记录
     * @param package_name 包名
     * @return 是否删除成功
     */
    bool removePackageRecord(const std::string& package_name);
    
    /**
     * @brief 显示指定包的所有文件路径
     * @param package_name 包名
     */
    void showPackageFiles(const std::string& package_name) const;
    
    /**
     * @brief 显示所有已安装的包
     */
    void showAllPackages() const;
    
    /**
     * @brief 保存记录到文件
     * @return 是否保存成功
     */
    bool saveToFile() const;
    
    /**
     * @brief 从文件加载记录
     * @return 是否加载成功
     */
    bool loadFromFile();

private:
    std::string record_file_path_;
    
    // 包信息结构
    struct PackageInfo {
        std::string install_path;
        std::vector<std::string> files;
    };
    
    // 包名 -> 包信息
    std::map<std::string, PackageInfo> packages_;
    
    /**
     * @brief 确保记录文件存在
     */
    void ensureRecordFileExists() const;
};

} // namespace Recorder
