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


//TODO: для приема символов через UART будем использовать gap буффер/связный список.

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
    Level2Command* level2Commands;
} Level1Command;


static Level2Command ADC_COMMANDS[2] = {
    {"setpsc",
        HAS_VAL,
        200
    },
    {"getval",
        NO_VAL,
        201
    }
};

static Level2Command TIMER_COMMANDS[2] = {
    {"setpsc",
        HAS_VAL,
        300
    },
    {"getpsc",
        NO_VAL,
        301
    }
};



// Глобальный массив команд первого уровня (инициализирован статически)
static Level1Command lvl1_commands[MAX_LEVEL1_COMMANDS] = {
    {   "adc",
        NO_CHAR,
        HAS_NUM,
        NOT_FIRST_LVL,
        2,
        ADC_COMMANDS
    },
    {
        "timer",
        NO_CHAR,
        HAS_NUM,
        NOT_FIRST_LVL,
        2,
        TIMER_COMMANDS
    },
    {   "info",
        NO_CHAR,
        NO_NUM,
        50,
        0,
        NULL
    },
    {
        "gpio",
        HAS_CHAR,
        NO_NUM,
        NOT_FIRST_LVL,
        0,
        NULL
    }
    // ... другие команды первого уровня
};



static uint16_t parse_lvl1(const char* user_command, uint16_t* lvl1_index)
{
    uint16_t user_index = 0;
    uint8_t flag;
    uint16_t j = 0;
    uint16_t i = 0;
    if (!(*user_command)) return 0;

    /* Ищем комманду первого уровня */

    for (i = 0; i < MAX_LEVEL1_COMMANDS; i++)
    {
        flag = 0;
        user_index = 0;
        j = 0;
        //До тех пор, пока у нас не конец комманды пользователя и не конец комманды из таблицы
        while(lvl1_commands[i].command_lvl1[j] && user_command[j] != '.' && user_command[j])
        {
            if(user_command[j] == lvl1_commands[i].command_lvl1[j]) //Если у нас совпадают символы
            {
                flag = 1;
                user_index++;
            }
            else
            {
                flag = 0;
                break;
            }
            j++;
        }

        if (flag && !lvl1_commands[i].command_lvl1[j] && (user_command[j] == '.' || !user_command[j]))
            break;
        else
            flag = 0;
    }
    if (!flag) return 0;
    *lvl1_index = i;
    //Решение костыльное. Мы его используем, т.к. ошибка должна вернуть 0, то нулевой индекс должен отличаться от 0
    return (user_index + 1);
}


uint16_t parse_getnum(uint16_t lvl1_index, const char* user_command, uint16_t user_index, uint8_t* number)
{
    uint16_t j = 0;
    if (lvl1_commands[lvl1_index].has_number || lvl1_commands[lvl1_index].has_char)
    {
        if (!user_command[user_index]) return 0; //В комманду с передаваемым номером он не был передан/передан неверно
        user_index++;   //Теперь user_command[user_index] ссылается на значение после точки
        if (user_command[user_index] == '0') return 0;   //Передали в неправильном формате. 0 не может идти первым.
        while(user_command[user_index] && user_command[user_index] != '.')
        {
            if (j > 1 && lvl1_commands[lvl1_index].has_number) return 0; //Передали слишком большое значение. Они у нас <= 99
            if (j > 0 && lvl1_commands[lvl1_index].has_char) return 0;   //Передали больше, чем один символ
            if (lvl1_commands[lvl1_index].has_number)
            {
                if (user_command[user_index] <= '9' && user_command[user_index] >= '0')
                {
                    switch (j)
                    {
                        case 0:
                            *number += (user_command[user_index + 1] && user_command[user_index + 1] != '.')
                            ? (user_command[user_index] - '0') * 10 : user_command[user_index] - '0';
                            break;
                        case 1:
                            *number += (user_command[user_index] - '0');
                            break;
                    }
                }
                else
                {
                    return 0; //Передали не число
                }
            }
            else if (lvl1_commands[lvl1_index].has_char)
            {
                *number = (user_command[user_index] >= 'a' && user_command[user_index] <= 'h')
                ? user_command[user_index] : 0;
                if (!(*number)) return 0;                   //Передали неверную букву
            }
            user_index++;
            j++;
            printf("%d\n", *number);
        }
        return user_index;
        printf("Second part: user_index = %d\nuser_command[user_index] = %c\n", user_index, user_command[user_index]);
    }
    else
        return 0;
}


uint8_t parse_lvl2(uint16_t lvl1_index, uint8_t* lvl2_index, const char* user_command, uint16_t user_index)
{
    uint16_t j = 0;
    uint16_t i = 0;
    uint8_t flag = 0;
    for (i = 0; i < lvl1_commands[lvl1_index].number_of_commands; i++)
    {
        flag = 0;
        *lvl2_index = i;
        j = 0;
        //До тех пор, пока у нас не конец комманды пользователя и не конец комманды из таблицы
        while(lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j] &&
            user_command[j + user_index] != '.' && user_command[j + user_index])
        {
            //Если у нас совпадают символы
            if(user_command[j + user_index] == lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j])
            {
                flag = 1;
            }
            else
            {
                flag = 0;
                break;
            }
            j++;
        }
        if (flag && !lvl1_commands[lvl1_index].level2Commands[i].command_lvl2[j]
            && (user_command[j + user_index] == '.' || !user_command[j + user_index]))
            break;
        else
            flag = 0;
    }
    if (!flag) return 0;
    user_index += j;
    return user_index;
}


uint8_t parse_getval(const char* user_command, uint16_t user_index, uint16_t* value)
{
    uint16_t i = 0;
    while (user_command[user_index + i])
    {
        if (user_command[user_index + i] < '0' && user_command[user_index + i] > '9')
            return 0; //Отправили неверный символ
        i++;
        if (i > 5)
        {
            printf("Было передано слишком большое значение\n");
            return 0;
        }
    }
    printf("i = %d\n", i);

    switch(i)
    {
        case 5:
            printf("In 5th section\n");
            *value = (user_command[user_index] - '0') * 10000 + (user_command[user_index + 1] - '0') * 1000
            + (user_command[user_index + 2] - '0') * 100 + (user_command[user_index + 3] - '0') * 10
            + (user_command[user_index + 4] - '0');
            return i;
            break;
        case 4:
            printf("In 4th section\n");
            *value = (user_command[user_index] - '0') * 1000 + (user_command[user_index + 1] - '0') * 100
            + (user_command[user_index + 2] - '0') * 10 + (user_command[user_index + 3] - '0');
            return i;
            break;
        case 3:
            printf("In 3trd section\n");
            *value = (user_command[user_index] - '0') * 100 + (user_command[user_index + 1] - '0') * 10
            + (user_command[user_index + 2] - '0');
            return i;
            break;
        case 2:
            *value = (user_command[user_index] - '0') * 10 + (user_command[user_index + 1] - '0');
            return i;
            break;
        case 1:
            *value = user_command[user_index] - '0';
            return i;
            break;
    }
    return 0;
}

// Функция для парсинга команды
int parse_command(const char* user_command, uint8_t* number, uint16_t* value)
{
    uint16_t lvl1_index;
    uint8_t lvl2_index;
    uint16_t user_index = 0;
    uint8_t flag;

    /* Ищем комманду первого уровня */
    user_index = parse_lvl1(user_command, &lvl1_index);
    if (!user_index) return 0; //Неверная комманда первого уровня
    user_index--;              //Функция парсинга возвращает индекс на 1 больше.

    /* По итогу нашли user_index - место конца комманды / '.', а также lvl1_index - индекс первоуровневой комманды. */

    if (lvl1_commands[lvl1_index].first_lvl_return_value && !user_command[user_index])
        return lvl1_commands[lvl1_index].first_lvl_return_value;
    user_index = parse_getnum(lvl1_index, user_command, user_index, number);
    if (!user_index || !user_command[user_index]) return 0;
    user_index++;
    user_index = parse_lvl2(lvl1_index, &lvl2_index, user_command, user_index);
    if (!user_index) return 0;
    if (!lvl1_commands[lvl1_index].level2Commands[lvl2_index].has_value && !user_command[user_index])
        return lvl1_commands[lvl1_index].level2Commands[lvl2_index].return_value;
    if (!lvl1_commands[lvl1_index].level2Commands[lvl2_index].has_value && user_command)
        return 0;

    if (!user_command[user_index]) return 0;
    user_index++;
    if (!user_command[user_index]) return 0;
    uint8_t digits_num = parse_getval(user_command, user_index, value);
    if (!digits_num) return 0; //Что-то пошло явно не так.
    return 1;
}

//CLI - интерпратор командной строки

int main()
{
    uint16_t tested;
    uint8_t tmp;
    char test[20];
    //scanf("%s", test);
    printf("Result of parsing is %d\n", parse_command("info", &tmp, &tested));
    printf("number = %d\nvalue = %d\n", tmp, tested);
    return 0;
}
