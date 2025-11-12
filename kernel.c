// ================================
// Базовые функции ввода-вывода
// ================================

void putchar(char c) {
    asm volatile (
        "mov $0x0E, %%ah\n"
        "mov %0, %%al\n"
        "int $0x10\n"
        :
        : "r"(c)
        : "ax"
    );
}

void print_string(const char* str) {
    while (*str) {
        putchar(*str);
        str++;
    }
}

void print_number(int num) {
    if (num == 0) {
        putchar('0');
        return;
    }

    char buffer[16];
    char* p = buffer + 15;
    *p = '\0';

    while (num > 0) {
        p--;
        *p = '0' + (num % 10);
        num /= 10;
    }

    print_string(p);
}

int check_key() {
    unsigned char result;
    asm volatile (
        "mov $0x01, %%ah\n"
        "int $0x16\n"
        "setnz %0\n"
        : "=r"(result)
        :
        : "ah", "al"
    );
    return result;
}

char get_key() {
    char result;
    asm volatile (
        "mov $0x00, %%ah\n"
        "int $0x16\n"
        "mov %%al, %0\n"
        : "=r"(result)
        :
        : "ah"
    );
    return result;
}

// ================================
// Функции задержки
// ================================

// Быстрая задержка для ввода в шелле
void fast_delay() {
    for (volatile int i = 0; i < 10000; i++); // Уменьшено в 50 раз
}

// Медленная задержка для вывода чисел в задачах
void slow_delay() {
    for (volatile int i = 0; i < 500000; i++);
}

// ================================
// Строковые функции
// ================================

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int strlen(const char* str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

// ================================
// Файловая система и менеджер задач
// ================================

#define MAX_TASKS 10
#define MAX_FILENAME 20
#define MAX_COMMAND_LENGTH 50

enum task_state {
    TASK_STOPPED,
    TASK_RUNNING,
    TASK_PAUSED
};

struct task {
    int id;
    char name[MAX_FILENAME];
    int counter;
    enum task_state state;
    void (*entry_point)();
};

struct task tasks[MAX_TASKS];
int task_count = 0;
struct task started_tasks[MAX_TASKS];
int started_task_count = 0;
int current_task_id = -1; // -1 означает, что активен шелл

// ================================
// Программы-задачи
// ================================

void odd_counter() {
    struct task* t = &started_tasks[current_task_id];
    if (t->counter == 0) { t->counter = 1; }
    print_string("ODD: ");
    print_number(t->counter);
    print_string(" ");
    t->counter += 2;
}

void even_counter() {
    struct task* t = &tasks[current_task_id];
    print_string("EVEN: ");
    print_number(t->counter);
    print_string(" ");
    t->counter += 2;
}

void test_program() {
    struct task* t = &tasks[current_task_id];
    print_string("TEST[");
    print_number(t->counter);
    print_string("] ");
    t->counter++;
}

// ================================
// Регистрация программ в ФС
// ================================

void register_task(const char* name, void (*entry)()) {
    if (task_count < MAX_TASKS) {
        tasks[task_count].id = task_count;
        strcpy(tasks[task_count].name, name);
        tasks[task_count].counter = 0;
        tasks[task_count].state = TASK_STOPPED;
        tasks[task_count].entry_point = entry;
        task_count++;
    }
}

void init_filesystem() {
    register_task("odd", odd_counter);
    register_task("even", even_counter);
    register_task("test", test_program);
}

// ================================
// Менеджер задач
// ================================

int find_task(const char* name) {
    for (int i = 0; i < task_count; i++) {
        if (strcmp(tasks[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void start_task(int task_id) {
    if (task_id >= 0 && task_id < task_count) {
        tasks[task_id].state = TASK_RUNNING;
        current_task_id = task_id;
        print_string("\r\nStarted '");
        print_string(tasks[task_id].name);
        print_string("'. Press SPACE to pause.\r\n");
    }
}

void pause_current_task() {
    if (current_task_id >= 0) {
        tasks[current_task_id].state = TASK_PAUSED;
        print_string("\r\nPaused '");
        print_string(tasks[current_task_id].name);
        print_string("'. Returning to shell.\r\n");
        current_task_id = -1;
    }
}

void resume_task(int task_id) {
    if (task_id >= 0 && task_id < task_count) {
        tasks[task_id].state = TASK_RUNNING;
        current_task_id = task_id;
        print_string("\r\nResumed '");
        print_string(tasks[task_id].name);
        print_string("'. Press SPACE to pause.\r\n");
    }
}

// ================================
// Шелл и команды
// ================================

void print_prompt() {
    print_string("\r\nOS> ");
}

void cmd_help() {
    print_string("\r\nAvailable commands:");
    print_string("\r\n  ls    - List available programs");
    print_string("\r\n  run   - Start or resume a program");
    print_string("\r\n  stop  - Stop a program");
    print_string("\r\n  ps    - Show running programs");
    print_string("\r\n  clear - Clear screen");
    print_string("\r\n  help  - Show this help");
    print_string("\r\n");
}

void cmd_ls() {
    print_string("\r\nAvailable programs:");
    for (int i = 0; i < task_count; i++) {
        print_string("\r\n  ");
        print_string(tasks[i].name);
    }
    print_string("\r\n");
}

void cmd_run(const char* name) {
    int task_id = find_task(name);
    if (task_id == -1) {
        print_string("\r\nProgram '");
        print_string(name);
        print_string("' not found.\r\n");
        return;
    }

    if (tasks[task_id].state == TASK_STOPPED) {
        start_task(task_id);
    } else if (tasks[task_id].state == TASK_PAUSED) {
        resume_task(task_id);
    } else {
        print_string("\r\nProgram '");
        print_string(name);
        print_string("' is already running.\r\n");
    }
}

void cmd_stop(const char* name) {
    int task_id = find_task(name);
    if (task_id == -1) {
        print_string("\r\nProgram '");
        print_string(name);
        print_string("' not found.\r\n");
        return;
    }

    tasks[task_id].state = TASK_STOPPED;
    tasks[task_id].counter = 0;

    if (current_task_id == task_id) {
        current_task_id = -1;
    }

    print_string("\r\nStopped '");
    print_string(name);
    print_string("'\r\n");
}

void cmd_ps() {
    print_string("\r\nRunning programs:");
    int found = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state != TASK_STOPPED) {
            print_string("\r\n  ");
            print_string(tasks[i].name);
            print_string(" - ");

            if (tasks[i].state == TASK_RUNNING) {
                print_string("RUNNING");
            } else {
                print_string("PAUSED");
            }

            print_string(" (counter: ");
            print_number(tasks[i].counter);
            print_string(")");
            found = 1;
        }
    }

    if (!found) {
        print_string("\r\n  No running programs");
    }
    print_string("\r\n");
}

void cmd_clear() {
    // Простая очистка экрана - выводим много новых строк
    for (int i = 0; i < 50; i++) {
        print_string("\r\n");
    }
}

void process_command(const char* command) {
    if (strcmp(command, "help") == 0) {
        cmd_help();
    } else if (strcmp(command, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(command, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(command, "clear") == 0) {
        cmd_clear();
    } else if (strncmp(command, "run ", 4) == 0) {
        cmd_run(command + 4);
    } else if (strncmp(command, "stop ", 5) == 0) {
        cmd_stop(command + 5);
    } else if (strlen(command) > 0) {
        print_string("\r\nUnknown command: '");
        print_string(command);
        print_string("'. Type 'help' for available commands.\r\n");
    }
}

// ================================
// Главная функция
// ================================

void kernel_main() {
    // Инициализация
    init_filesystem();

    // Приветствие
    print_string("AivaOS Shell v1.0");
    print_string("\r\nType 'help' for commands");

    char command[MAX_COMMAND_LENGTH];
    int command_index = 0;
    int last_key_state = 0;
    int numbers_printed = 0;

    while (1) {
        // Режим шелла
        if (current_task_id == -1) {
            print_prompt();
            command_index = 0;
            command[0] = '\0';

            // Чтение команды
            while (1) {
                if (check_key()) {
                    char key = get_key();

                    if (key == '\r') { // Enter
                        putchar('\r');
                        putchar('\n');
                        process_command(command);
                        break;
                    } else if (key == 8 || key == 127) { // Backspace
                        if (command_index > 0) {
                            command_index--;
                            command[command_index] = '\0';
                            putchar('\b');
                            putchar(' ');
                            putchar('\b');
                        }
                    } else if (key >= 32 && key <= 126) { // Печатные символы
                        if (command_index < MAX_COMMAND_LENGTH - 1) {
                            command[command_index++] = key;
                            command[command_index] = '\0';
                            putchar(key);
                        }
                    }
                }
                fast_delay();
            }
        }
        // Режим выполнения задачи
        else {
            struct task* current = &tasks[current_task_id];

            // Проверка паузы (пробел)
            int key_state = check_key();
            if (key_state && !last_key_state) {
                char key = get_key();
                if (key == ' ') {
                    pause_current_task();
                    continue;
                }
            }
            last_key_state = key_state;

            // Выполнение текущей задачи
            if (current->state == TASK_RUNNING) {
                current->entry_point();
                numbers_printed++;

                // Перенос строки для читаемости
                if (numbers_printed >= 8) {
                    print_string("\r\n");
                    numbers_printed = 0;
                }

                slow_delay();
            }
        }
    }
}