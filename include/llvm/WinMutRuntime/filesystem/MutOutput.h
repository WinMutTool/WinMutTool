#include <map>
#include <vector>
#include <set>
#include <algorithm>

namespace accmut {
    class MutOutput{
        int createdStdoutFileFor0;  // 为原始进程创建的stdout输出文件的文件描述符，只应当在原始进程中使用
        // std::map<int, int> createdStdoutFileForN0;  // <mut_id, fd>
        
        // TODO: 或许可以再添加一个 set 来记录当前进程分支已经打开的文件来减少查找时间，小优化
        // std::map<int, std::map<int, std::string>> openedMutOutputFile; // <mut_id, <fd, std::string>>
        std::map<int, std::map<std::string, int>> openedMutOutputFile; // <mut_id, <std::string, fd>>

        int copy_from;
        std::vector<int> eq_class_mut_id;

        std::map<std::string, int> openedFileMap_path2fd;
        std::map<int, std::string> openedFileMap_fd2path;
        std::set<std::string>      openedFileSet;

        // MutOutput(){
        //     createdStdoutFileFor0 = -1;
        // };
        MutOutput(){
            createdStdoutFileFor0 = open_stdoutcopy();
        };
        static MutOutput* holdptr;
        public:
            // static MutOutput* getInstance(){
            //     static MutOutput* instanceptr = new MutOutput();
            //     return instanceptr;
            // }
            static MutOutput* getInstance();
            static void hold();

            // void createStdoutFileFor0();
            // void createStdoutFileForN0(std::vector<int> eq_class_mut_id);
            // void writeStdoutFile(const void *buf, size_t count);
            // static bool isStdout(int fd);
            // bool isOri();

            // static bool isRegFile(int fd);

            int open_stdoutcopy();
            void open_and_register_MutOutputFile(const char* filepath, int fd, int flags, int mode);
            void prepare_copy(int copy_from, std::vector<int> mut_id_need_split);
            void copy_and_register_MutOutputFile(std::vector<int> eq_class_mut_id);
            int  copy_MutOutputFile(int sourceFd, const char* destinationPath);
            void write_registered_MutOutputFile(int fd, const void *buf, size_t count);
    };

}