#include <stdio.h>
#include <stdint.h>
#define MAX_LEVEL1_COMMANDS 4 //Число комманд первого уровня
#define NO_NUM 0
#define HAS_NUM 1
#define HAS_CHAR 1
#define NO_CHAR 0
#define HAS_VAL 1
#define NO_VAL 0
#define NOT_FIRST_LVL 0


#define DEBUG_LVL_1

// Структура для хранения команды второго уровня и связанного с ней значения
typedef struct {
    const char* command_lvl2;
    uint16_t has_value : 1;
    uint16_t return_value : 15;
} Level2Command;



// Структура для хранения команды первого уровня и массива команд второго уровня
typedef struct {
    const char* command_lvl1;
    uint8_t has_char : 1;
    uint8_t has_number : 1;
    uint8_t first_lvl_return_value : 6;
    uint8_t number_of_commands;
    const Level2Command* level2Commands;
} Level1Command;



// Команды второго уровня для ADC
static const Level2Command ADC_COMMANDS[2] = {
    {
        .command_lvl2 = "setpsc",
        .has_value = 1,
        .return_value = 200
    },
    {
        .command_lvl2 = "getval",
        .has_value = 0,
        .return_value = 201
    }
};


static const Level2Command TIMER_COMMANDS[2] = {
    {   .command_lvl2 = "setpsc",
        .has_value = HAS_VAL,
        .return_value = 300
    },
    {   .command_lvl2 = "getpsc",
        .has_value = NO_VAL,
        .return_value = 301
    }
};




// Глобальный массив команд первого уровня (инициализирован статически)
static Level1Command lvl1_commands[MAX_LEVEL1_COMMANDS] = {
    {   .command_lvl1 = "adc",
        .has_char = NO_CHAR,
        .has_number = HAS_NUM,
        .first_lvl_return_value = NOT_FIRST_LVL,
        .number_of_commands = 2,
        .level2Commands = ADC_COMMANDS
    },
    {
        .command_lvl1 = "timer",
        .has_char = NO_CHAR,
        .has_number = HAS_NUM,
        .first_lvl_return_value = NOT_FIRST_LVL,
        .number_of_commands = 2,
        .level2Commands = TIMER_COMMANDS
    },
    {   .command_lvl1 = "info",
        .has_char = NO_CHAR,
        .has_number = HAS_NUM,
        .first_lvl_return_value = 50,
        .number_of_commands = 0,
        .level2Commands = NULL
    }, //Делаю
    {
        .command_lvl1 = "gpio",
        .has_char = HAS_CHAR,
        .has_number = NO_NUM,
        .first_lvl_return_value = 55,
        .number_of_commands = 0,
        .level2Commands = NULL
    }
    // ... другие команды первого уровня
};


//Создаем динамически указатель на структуру и динамически статический менеджер этих датчиков. Делаем также два обработчика - для EXTI5-9 и нормального EXTI. В ините модуля добавляем новый датчик в менеджер датчиков, также включаем прерывание. Структра представляет из себя: trigger_pin, echo_pin, time_before_measure, time_after_measure, distance_before_calc, irqn_type



// Парсит команду первого уровня
static uint16_t parse_lvl1(const char* user_command, uint16_t* lvl1_index)
{
    if (!user_command[0]) return 0;

    for (uint16_t i = 0; i < MAX_LEVEL1_COMMANDS; i++)
    {
        uint16_t j = 0;
        while (lvl1_commands[i].command_lvl1[j] && user_command[j] != '.' && user_command[j])
        {
            if (user_command[j] != lvl1_commands[i].command_lvl1[j]) break;
            j++;
        }
        if (!lvl1_commands[i].command_lvl1[j] && (user_command[j] == '.' || !user_command[j]))
        {
            *lvl1_index = i;
            return j + 1; // Возвращаем индекс символа после команды
        }
    }
    return 0; // Команда не найдена
}

// Парсит число или символ после точки
static uint16_t parse_getnum(uint16_t lvl1_index, const char* user_command, uint16_t user_index, uint8_t* number)
{
    if (!(lvl1_commands[lvl1_index].has_number || lvl1_commands[lvl1_index].has_char)) return 0;
    if (!user_command[user_index]) return 0; // Нет аргумента после команды

    user_index++;
    if (!user_command[user_index] || user_command[user_index] == '0') return 0;

    uint16_t j = 0;
    while (user_command[user_index] && user_command[user_index] != '.')
    {
        if ((j > 1 && lvl1_commands[lvl1_index].has_number) || (j > 0 && lvl1_commands[lvl1_index].has_char)) return 0;

        if (lvl1_commands[lvl1_index].has_number)
        {
            if (user_command[user_index] < '0' || user_command[user_index] > '9') return 0;
            *number = *number * 10 + (user_command[user_index] - '0');
        }
        else if (lvl1_commands[lvl1_index].has_char)
        {
            if (user_command[user_index] < 'a' || user_command[user_index] > 'h') return 0;
            *number = user_command[user_index];
        }
        user_index++;
        j++;
    }
    return user_index;
}

// Парсит команду второго уровня
static uint8_t parse_lvl2(uint16_t lvl1_index, uint8_t* lvl2_index, const char* user_command, uint16_t user_index)
{
    for (uint16_t i = 0; i < lvl1_commands[lvl1_index].number_of_commands; i++)
    {
        uint16_t j = 0;
        while (lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j] &&
            user_command[user_index + j] != '.' && user_command[user_index + j])
        {
            if (user_command[user_index + j] != lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j]) break;
            j++;
        }
            if (!lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j] &&
                (user_command[user_index + j] == '.' || !user_command[user_index + j]))
            {
                *lvl2_index = i;
                return user_index + j;
            }
    }
    return 0; // Команда второго уровня не найдена
}

// Парсит числовое значение после команды второго уровня
static uint8_t parse_getval(const char* user_command, uint16_t user_index, uint16_t* value)
{
    uint16_t i = 0;
    *value = 0;
    while (user_command[user_index + i])
    {
        if (user_command[user_index + i] < '0' || user_command[user_index + i] > '9') return 0;
        i++;
        if (i > 5) return 0; // Слишком большое значение
        *value = *value * 10 + (user_command[user_index + i - 1] - '0');
    }
    return i;
}

// Функция для парсинга команды
int cli_parse_command(const char* user_command, uint8_t* number, uint16_t* value)
{
    uint16_t lvl1_index;
    uint8_t lvl2_index;
    *number = 0; // Инициализируем number
    *value = 0;   // Инициализируем value

    uint16_t user_index = parse_lvl1(user_command, &lvl1_index);
    if (!user_index)
        return 0; // Неверная команда первого уровня
    user_index--;

    if (lvl1_commands[lvl1_index].first_lvl_return_value)
    {
        if (!(lvl1_commands[lvl1_index].has_char || lvl1_commands[lvl1_index].has_number))
        {
            if (!user_command[user_index]) return lvl1_commands[lvl1_index].first_lvl_return_value;
            else return 0; // Аргументы переданы для команды, которая их не принимает
        }
    }

    user_index = parse_getnum(lvl1_index, user_command, user_index, number);
    if (lvl1_commands[lvl1_index].first_lvl_return_value)
    {
        if (user_index && !user_command[user_index]) return lvl1_commands[lvl1_index].first_lvl_return_value;
        else return 0; // Неверный аргумент для команды первого уровня/не был передан агрумент
    }

    if (!user_index || !user_command[user_index]) return 0; // Нет команды второго уровня
    user_index++;

    user_index = parse_lvl2(lvl1_index, &lvl2_index, user_command, user_index);
    if (!user_index) return 0; // Неверная команда второго уровня

    if (!lvl1_commands[lvl1_index].level2Commands[lvl2_index].has_value)
    {
        if (!user_command[user_index]) return lvl1_commands[lvl1_index].level2Commands[lvl2_index].return_value;
        else return 0; // Аргументы переданы для команды, которая их не принимает
    }

    if (!user_command[user_index]) return 0; // Нет значения после точки
    user_index++;

    if (!parse_getval(user_command, user_index, value)) return 0; // Неверное значение
    return lvl1_commands[lvl1_index].level2Commands[lvl2_index].return_value;
}

// CLI - интерпретатор командной строки
int main()
{
    uint16_t tested;
    uint8_t tmp;
    printf("Result of parsing is %d\n", cli_parse_command("info.1", &tmp, &tested));
    printf("number = %d\nvalue = %d\n", tmp, tested);

    printf("Result of parsing is %d\n", cli_parse_command("adc.99.setpsc.1223", &tmp, &tested));
    printf("number = %d\nvalue = %d\n", tmp, tested);

    printf("Result of parsing is %d\n", cli_parse_command("timer.1.getpsc", &tmp, &tested));
    printf("number = %d\nvalue = %d\n", tmp, tested);

    printf("Result of parsing is %d\n", cli_parse_command("gpio.f", &tmp, &tested));
    printf("number = %c\nvalue = %d\n", tmp, tested);

    return 0;

    //TODO: Убрать лишние проверки из parse_getnum
}
