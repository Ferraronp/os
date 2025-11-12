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

void fast_delay() {
    for (volatile int i = 0; i < 10000; i++);
}

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

#define MAX_TASKS 20
#define MAX_TASK_INSTANCES 10
#define MAX_FILENAME 20
#define MAX_COMMAND_LENGTH 50

enum task_state {
    TASK_STOPPED,
    TASK_RUNNING,
    TASK_PAUSED,
    TASK_QUEUED
};

struct task_type {
    int id;
    char name[MAX_FILENAME];
    void (*entry_point)();
};

struct task_instance {
    int id;
    int type_id;
    char name[MAX_FILENAME];
    int counter;
    enum task_state state;
    int queue_position;
};

struct task_type task_types[MAX_TASKS];
int task_type_count = 0;
struct task_instance task_instances[MAX_TASK_INSTANCES];
int task_instance_count = 0;
int current_task_id = -1;
int queue_head = 0;
int queue_tail = 0;
int task_queue[MAX_TASK_INSTANCES];
int queue_running = 0; // Флаг выполнения очереди

// ================================
// Программы-задачи
// ================================

void odd_counter() {
    struct task_instance* t = &task_instances[current_task_id];
    if (t->counter == 0) { t->counter = 1; }
    print_string("ODD[");
    print_number(t->id);
    print_string("]: ");
    print_number(t->counter);
    print_string(" ");
    t->counter += 2;
}

void even_counter() {
    struct task_instance* t = &task_instances[current_task_id];
    print_string("EVEN[");
    print_number(t->id);
    print_string("]: ");
    print_number(t->counter);
    print_string(" ");
    t->counter += 2;
}

void test_program() {
    struct task_instance* t = &task_instances[current_task_id];
    print_string("TEST[");
    print_number(t->id);
    print_string("]: ");
    print_number(t->counter);
    print_string(" ");
    t->counter++;
}

// ================================
// Регистрация программ в ФС
// ================================

void register_task_type(const char* name, void (*entry)()) {
    if (task_type_count < MAX_TASKS) {
        task_types[task_type_count].id = task_type_count;
        strcpy(task_types[task_type_count].name, name);
        task_types[task_type_count].entry_point = entry;
        task_type_count++;
    }
}

void init_filesystem() {
    register_task_type("odd", odd_counter);
    register_task_type("even", even_counter);
    register_task_type("test", test_program);
}

// ================================
// Менеджер задач
// ================================

int find_task_type(const char* name) {
    for (int i = 0; i < task_type_count; i++) {
        if (strcmp(task_types[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int create_task_instance(int type_id) {
    if (task_instance_count >= MAX_TASK_INSTANCES) {
        return -1;
    }

    struct task_instance* t = &task_instances[task_instance_count];
    t->id = task_instance_count;
    t->type_id = type_id;
    t->counter = 0;
    t->state = TASK_STOPPED;
    t->queue_position = -1;

    // Создаем уникальное имя для экземпляра
    strcpy(t->name, task_types[type_id].name);
    char id_str[5];
    char* p = id_str + 4;
    *p = '\0';
    int num = t->id;
    do {
        p--;
        *p = '0' + (num % 10);
        num /= 10;
    } while (num > 0);

    int len = strlen(t->name);
    t->name[len] = '_';
    strcpy(t->name + len + 1, p);

    return task_instance_count++;
}

void add_to_queue(int task_id) {
    if (task_instances[task_id].state == TASK_QUEUED) {
        return; // Уже в очереди
    }

    task_queue[queue_tail] = task_id;
    task_instances[task_id].state = TASK_QUEUED;
    task_instances[task_id].queue_position = queue_tail;
    queue_tail = (queue_tail + 1) % MAX_TASK_INSTANCES;
}

void remove_from_queue(int task_id) {
    if (task_instances[task_id].state != TASK_QUEUED) {
        return;
    }

    int pos = task_instances[task_id].queue_position;
    if (pos == -1) return;

    // Сдвигаем очередь
    for (int i = pos; i != queue_tail; i = (i + 1) % MAX_TASK_INSTANCES) {
        int next = (i + 1) % MAX_TASK_INSTANCES;
        task_queue[i] = task_queue[next];
        task_instances[task_queue[i]].queue_position = i;
    }

    queue_tail = (queue_tail - 1 + MAX_TASK_INSTANCES) % MAX_TASK_INSTANCES;
    task_instances[task_id].queue_position = -1;
    task_instances[task_id].state = TASK_STOPPED;
}

void start_task(int task_id) {
    if (task_id >= 0 && task_id < task_instance_count) {
        task_instances[task_id].state = TASK_RUNNING;
        current_task_id = task_id;
        print_string("\r\nStarted '");
        print_string(task_instances[task_id].name);
        print_string("'. Press SPACE to pause.\r\n");
    }
}

void pause_current_task() {
    if (current_task_id >= 0) {
        task_instances[current_task_id].state = TASK_PAUSED;
        print_string("\r\nPaused '");
        print_string(task_instances[current_task_id].name);
        print_string("'. Returning to shell.\r\n");
        current_task_id = -1;
    }
}

void resume_task(int task_id) {
    if (task_id >= 0 && task_id < task_instance_count) {
        task_instances[task_id].state = TASK_RUNNING;
        current_task_id = task_id;
        print_string("\r\nResumed '");
        print_string(task_instances[task_id].name);
        print_string("'. Press SPACE to pause.\r\n");
    }
}

void stop_queue() {
    queue_running = 0;
    print_string("\r\nQueue execution stopped.\r\n");
}

void run_all_queued() {
    if (queue_head == queue_tail) {
        print_string("\r\nQueue is empty.\r\n");
        return;
    }

    print_string("\r\nRunning all queued tasks (press ESC to stop)...\r\n");
    queue_running = 1;

    int original_head = queue_head;
    int iterations = 0;
    const int max_iterations_per_task = 5; // Ограничение итераций на задачу

    while (queue_running && queue_head != queue_tail) {
        // Проверка нажатия ESC для прерывания
        if (check_key()) {
            char key = get_key();
            if (key == 27) { // ESC
                stop_queue();
                break;
            }
        }

        int task_id = task_queue[queue_head];

        // Временно запускаем задачу для выполнения одной итерации
        task_instances[task_id].state = TASK_RUNNING;
        current_task_id = task_id;

        // Выполняем одну итерацию задачи
        task_types[task_instances[task_id].type_id].entry_point();
        print_string("\r\n");

        // Возвращаем задачу в состояние QUEUED
        task_instances[task_id].state = TASK_QUEUED;
        current_task_id = -1;

        // Перемещаем задачу в конец очереди
        queue_head = (queue_head + 1) % MAX_TASK_INSTANCES;
        task_queue[queue_tail] = task_id;
        queue_tail = (queue_tail + 1) % MAX_TASK_INSTANCES;

        iterations++;

        // Ограничиваем количество итераций для предотвращения бесконечного выполнения
        if (iterations >= (queue_tail - original_head + MAX_TASK_INSTANCES) % MAX_TASK_INSTANCES * max_iterations_per_task) {
            print_string("\r\nMaximum iterations reached. Stopping queue.\r\n");
            stop_queue();
            break;
        }

        slow_delay();
    }

    if (!queue_running) {
        print_string("Queue execution interrupted.\r\n");
    } else {
        print_string("All queued tasks completed.\r\n");
    }

    current_task_id = -1;
}

// ================================
// Шелл и команды
// ================================

void print_prompt() {
    print_string("\r\nOS> ");
}

void cmd_help() {
    print_string("\r\nAvailable commands:");
    print_string("\r\n  ls       - List available program types");
    print_string("\r\n  create   - Create new task instance");
    print_string("\r\n  run      - Start or resume a task");
    print_string("\r\n  stop     - Stop a task");
    print_string("\r\n  queue    - Add task to execution queue");
    print_string("\r\n  runqueue - Run all queued tasks");
    print_string("\r\n  stopqueue- Stop queue execution");
    print_string("\r\n  ps       - Show all task instances");
    print_string("\r\n  clear    - Clear screen");
    print_string("\r\n  help     - Show this help");
    print_string("\r\n");
}

void cmd_ls() {
    print_string("\r\nAvailable program types:");
    for (int i = 0; i < task_type_count; i++) {
        print_string("\r\n  ");
        print_string(task_types[i].name);
    }
    print_string("\r\n");
}

void cmd_create(const char* type_name) {
    int type_id = find_task_type(type_name);
    if (type_id == -1) {
        print_string("\r\nProgram type '");
        print_string(type_name);
        print_string("' not found.\r\n");
        return;
    }

    int instance_id = create_task_instance(type_id);
    if (instance_id == -1) {
        print_string("\r\nCannot create task - maximum instances reached.\r\n");
        return;
    }

    print_string("\r\nCreated task instance '");
    print_string(task_instances[instance_id].name);
    print_string("' with ID ");
    print_number(instance_id);
    print_string("\r\n");
}

void cmd_run(const char* id_str) {
    int task_id = 0;
    for (int i = 0; id_str[i]; i++) {
        if (id_str[i] >= '0' && id_str[i] <= '9') {
            task_id = task_id * 10 + (id_str[i] - '0');
        } else {
            print_string("\r\nInvalid task ID.\r\n");
            return;
        }
    }

    if (task_id < 0 || task_id >= task_instance_count) {
        print_string("\r\nTask instance ");
        print_number(task_id);
        print_string(" not found.\r\n");
        return;
    }

    if (task_instances[task_id].state == TASK_STOPPED) {
        start_task(task_id);
    } else if (task_instances[task_id].state == TASK_PAUSED) {
        resume_task(task_id);
    } else {
        print_string("\r\nTask '");
        print_string(task_instances[task_id].name);
        print_string("' is already running or queued.\r\n");
    }
}

void cmd_stop(const char* id_str) {
    int task_id = 0;
    for (int i = 0; id_str[i]; i++) {
        if (id_str[i] >= '0' && id_str[i] <= '9') {
            task_id = task_id * 10 + (id_str[i] - '0');
        } else {
            print_string("\r\nInvalid task ID.\r\n");
            return;
        }
    }

    if (task_id < 0 || task_id >= task_instance_count) {
        print_string("\r\nTask instance ");
        print_number(task_id);
        print_string(" not found.\r\n");
        return;
    }

    remove_from_queue(task_id);
    task_instances[task_id].state = TASK_STOPPED;
    task_instances[task_id].counter = 0;

    if (current_task_id == task_id) {
        current_task_id = -1;
    }

    print_string("\r\nStopped '");
    print_string(task_instances[task_id].name);
    print_string("'\r\n");
}

void cmd_queue(const char* id_str) {
    int task_id = 0;
    for (int i = 0; id_str[i]; i++) {
        if (id_str[i] >= '0' && id_str[i] <= '9') {
            task_id = task_id * 10 + (id_str[i] - '0');
        } else {
            print_string("\r\nInvalid task ID.\r\n");
            return;
        }
    }

    if (task_id < 0 || task_id >= task_instance_count) {
        print_string("\r\nTask instance ");
        print_number(task_id);
        print_string(" not found.\r\n");
        return;
    }

    add_to_queue(task_id);
    print_string("\r\nAdded '");
    print_string(task_instances[task_id].name);
    print_string("' to execution queue\r\n");
}

void cmd_runqueue() {
    run_all_queued();
}

void cmd_stopqueue() {
    stop_queue();
}

void cmd_ps() {
    print_string("\r\nTask instances:");
    int found = 0;
    for (int i = 0; i < task_instance_count; i++) {
        print_string("\r\n  ");
        print_number(task_instances[i].id);
        print_string(": ");
        print_string(task_instances[i].name);
        print_string(" - ");

        switch (task_instances[i].state) {
            case TASK_STOPPED: print_string("STOPPED"); break;
            case TASK_RUNNING: print_string("RUNNING"); break;
            case TASK_PAUSED: print_string("PAUSED"); break;
            case TASK_QUEUED:
                print_string("QUEUED (pos: ");
                print_number(task_instances[i].queue_position);
                print_string(")");
                break;
        }

        print_string(" (counter: ");
        print_number(task_instances[i].counter);
        print_string(")");
        found = 1;
    }

    if (!found) {
        print_string("\r\n  No task instances created");
    }
    print_string("\r\n");
}

void cmd_clear() {
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
    } else if (strcmp(command, "runqueue") == 0) {
        cmd_runqueue();
    } else if (strcmp(command, "stopqueue") == 0) {
        cmd_stopqueue();
    } else if (strncmp(command, "create ", 7) == 0) {
        cmd_create(command + 7);
    } else if (strncmp(command, "run ", 4) == 0) {
        cmd_run(command + 4);
    } else if (strncmp(command, "stop ", 5) == 0) {
        cmd_stop(command + 5);
    } else if (strncmp(command, "queue ", 6) == 0) {
        cmd_queue(command + 6);
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
    print_string("AivaOS Shell v2.0");
    print_string("\r\nType 'help' for commands");

    char command[MAX_COMMAND_LENGTH];
    int command_index = 0;
    int last_key_state = 0;
    int numbers_printed = 0;

    while (1) {
        // Режим шелла
        if (current_task_id == -1 && !queue_running) {
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
        // Режим выполнения очереди
        else if (queue_running) {
            // Проверка нажатия ESC для прерывания очереди
            if (check_key()) {
                char key = get_key();
                if (key == 27) { // ESC
                    stop_queue();
                    continue;
                }
            }

            // Очередь выполняется в функции run_all_queued()
            // Здесь мы просто ждем завершения
            fast_delay();
        }
        // Режим выполнения одной задачи
        else {
            struct task_instance* current = &task_instances[current_task_id];

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
                task_types[current->type_id].entry_point();
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