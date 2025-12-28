#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>

class SharedLibLauncher {
public:
    static int launch(const std::string& libraryPath) {
        // Загружаем библиотеку
        void* handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Ошибка загрузки библиотеки: " << dlerror() << std::endl;
            return -1;
        }

        // Ищем точку входа (обычно main или run)
        typedef int (*EntryPoint)();

        // Пробуем разные возможные имена функций входа
        EntryPoint entry = nullptr;
        const char* possibleEntries[] = {"main", "run", "start", "execute"};

        for (const char* entryName : possibleEntries) {
            entry = (EntryPoint)dlsym(handle, entryName);
            if (entry) {
                break;
            }
        }

        if (!entry) {
            std::cerr << "Не найдена точка входа в библиотеке" << std::endl;
            dlclose(handle);
            return -1;
        }

        // Вызываем точку входа
        int result = entry();

        // Закрываем библиотеку
        dlclose(handle);

        return result;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <путь_к_библиотеке>" << std::endl;
        std::cerr << "Пример: " << argv[0] << " Consumer" << std::endl;
        return 1;
    }

    std::string libraryPath = argv[1];

    // Если путь не абсолютный, добавляем текущую директорию
    if (libraryPath.find('/') == std::string::npos) {
        libraryPath = "./" + libraryPath;
    }

    return SharedLibLauncher::launch(libraryPath);
}
