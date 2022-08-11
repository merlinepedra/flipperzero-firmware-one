#include "cli_command_gpio.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/toolbox/args.h>

void cli_command_gpio_print_usage() {
    printf("Usage:\r\n");
    printf("gpio <cmd> <args>\r\n");
    printf("Cmd list:\r\n");
    printf("\tmode <pin_name> <0|1>\t - Set gpio mode: 0 - input, 1 - output\r\n");
    printf("\tset <pin_name> <0|1>\t - Set gpio value\r\n");
    printf("\tread <pin_name>\t - Read gpio value\r\n");
}

static bool pin_name_to_int(string_t pin_name, size_t* result) {
    bool found = false;
    bool debug = furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug);
    for(size_t i = 0; i < gpio_pins_count; i++) {
        if(!string_cmp(pin_name, gpio_pins[i].name)) {
            if(!gpio_pins[i].debug || (gpio_pins[i].debug && debug)) {
                *result = i;
                found = true;
                break;
            }
        }
    }

    return found;
}

static void gpio_print_pins(void) {
    printf("Wrong pin name. Available pins: ");
    bool debug = furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug);
    for(size_t i = 0; i < gpio_pins_count; i++) {
        if(!gpio_pins[i].debug || (gpio_pins[i].debug && debug)) {
            printf("%s ", gpio_pins[i].name);
        }
    }
}

typedef enum { OK, ERR_CMD_SYNTAX, ERR_PIN, ERR_VALUE } GpioParseError;

static GpioParseError gpio_command_parse(string_t args, size_t* pin_num, uint8_t* value) {
    string_t pin_name;
    string_init(pin_name);

    size_t ws = string_search_char(args, ' ');
    if(ws == STRING_FAILURE) {
        return ERR_CMD_SYNTAX;
    }

    string_set_n(pin_name, args, 0, ws);
    string_right(args, ws);
    string_strim(args);

    if(!pin_name_to_int(pin_name, pin_num)) {
        string_clear(pin_name);
        return ERR_PIN;
    }

    string_clear(pin_name);

    if(!string_cmp(args, "0")) {
        *value = 0;
    } else if(!string_cmp(args, "1")) {
        *value = 1;
    } else {
        return ERR_VALUE;
    }

    return OK;
}

void cli_command_gpio_mode(Cli* cli, string_t args, void* context) {
    UNUSED(cli);
    UNUSED(context);

    size_t num = 0;
    uint8_t value = 255;

    GpioParseError err = gpio_command_parse(args, &num, &value);

    if(ERR_CMD_SYNTAX == err) {
        cli_print_usage("gpio mode", "<pin_name> <0|1>", string_get_cstr(args));
        return;
    } else if(ERR_PIN == err) {
        gpio_print_pins();
        return;
    } else if(ERR_VALUE == err) {
        printf("Value is invalid. Enter 1 for input or 0 for output");
        return;
    }

    if(gpio_pins[num].debug) {
        printf(
            "Changing this pin mode may damage hardware. Are you sure you want to continue? (y/n)?\r\n");
        char c = cli_getc(cli);
        if(c != 'y' && c != 'Y') {
            printf("Cancelled.\r\n");
            return;
        }
    }

    if(value == 1) { // output
        furi_hal_gpio_write(gpio_pins[num].pin, false);
        furi_hal_gpio_init_simple(gpio_pins[num].pin, GpioModeOutputPushPull);
        printf("Pin %s is now an output (low)", gpio_pins[num].name);
    } else { // input
        furi_hal_gpio_init_simple(gpio_pins[num].pin, GpioModeInput);
        printf("Pin %s is now an input", gpio_pins[num].name);
    }
}

void cli_command_gpio_read(Cli* cli, string_t args, void* context) {
    UNUSED(cli);
    UNUSED(context);

    size_t num = 0;
    if(!pin_name_to_int(args, &num)) {
        gpio_print_pins();
        return;
    }

    if(LL_GPIO_MODE_INPUT !=
       LL_GPIO_GetPinMode(gpio_pins[num].pin->port, gpio_pins[num].pin->pin)) {
        printf("Err: pin %s is not set as an input.", gpio_pins[num].name);
        return;
    }

    uint8_t val = !!furi_hal_gpio_read(gpio_pins[num].pin);

    printf("Pin %s <= %u", gpio_pins[num].name, val);
}

void cli_command_gpio_set(Cli* cli, string_t args, void* context) {
    UNUSED(context);

    size_t num = 0;
    uint8_t value = 0;
    GpioParseError err = gpio_command_parse(args, &num, &value);

    if(ERR_CMD_SYNTAX == err) {
        cli_print_usage("gpio set", "<pin_name> <0|1>", string_get_cstr(args));
        return;
    } else if(ERR_PIN == err) {
        gpio_print_pins();
        return;
    } else if(ERR_VALUE == err) {
        printf("Value is invalid. Enter 1 for high or 0 for low");
        return;
    }

    if(LL_GPIO_MODE_OUTPUT !=
       LL_GPIO_GetPinMode(gpio_pins[num].pin->port, gpio_pins[num].pin->pin)) {
        printf("Err: pin %s is not set as an output.", gpio_pins[num].name);
        return;
    }

    // Extra check if debug pins used
    if(gpio_pins[num].debug) {
        printf(
            "Setting this pin may damage hardware. Are you sure you want to continue? (y/n)?\r\n");
        char c = cli_getc(cli);
        if(c != 'y' && c != 'Y') {
            printf("Cancelled.\r\n");
            return;
        }
    }

    furi_hal_gpio_write(gpio_pins[num].pin, !!value);
    printf("Pin %s => %u", gpio_pins[num].name, !!value);
}

void cli_command_gpio(Cli* cli, string_t args, void* context) {
    string_t cmd;
    string_init(cmd);

    do {
        if(!args_read_string_and_trim(args, cmd)) {
            cli_command_gpio_print_usage();
            break;
        }

        if(string_cmp_str(cmd, "mode") == 0) {
            cli_command_gpio_mode(cli, args, context);
            break;
        }

        if(string_cmp_str(cmd, "set") == 0) {
            cli_command_gpio_set(cli, args, context);
            break;
        }

        if(string_cmp_str(cmd, "read") == 0) {
            cli_command_gpio_read(cli, args, context);
            break;
        }

        cli_command_gpio_print_usage();
    } while(false);

    string_clear(cmd);
}
