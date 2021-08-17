#include "subghz_protocol_princeton.h"
/*
 * Help
 * https://phreakerclub.com/447
 *
 */

#define SUBGHZ_PT_SHORT 450
#define SUBGHZ_PT_LONG (SUBGHZ_PT_SHORT * 3)
#define SUBGHZ_PT_GUARD (SUBGHZ_PT_SHORT * 30)

struct SubGhzEncoderPrinceton {
    uint32_t key;
    uint16_t te;
    size_t repeat;
    size_t front;
};

struct SubGhzDecoderPrinceton {
    SubGhzProtocolCommon common;
    uint16_t te;
};

SubGhzEncoderPrinceton* subghz_encoder_princeton_alloc() {
    SubGhzEncoderPrinceton* instance = furi_alloc(sizeof(SubGhzEncoderPrinceton));
    

    return instance;
}

void subghz_encoder_princeton_free(SubGhzEncoderPrinceton* instance) {
    furi_assert(instance);
    free(instance);
}
void subghz_encoder_princeton_set_te(SubGhzEncoderPrinceton* instance, void* decoder){
   SubGhzDecoderPrinceton* pricenton = decoder;
    if((pricenton->te) !=0){
        instance->te = pricenton->te;
    }else{
        instance->te = SUBGHZ_PT_SHORT;
    }
}


void subghz_encoder_princeton_reset(SubGhzEncoderPrinceton* instance, uint32_t key, size_t repeat) {
    furi_assert(instance);
    instance->te = SUBGHZ_PT_SHORT;
    instance->key = key;
    instance->repeat = repeat;
    instance->front = 48;
}

size_t subghz_encoder_princeton_get_repeat_left(SubGhzEncoderPrinceton* instance) {
    furi_assert(instance);
    return instance->repeat;
}

LevelDuration subghz_encoder_princeton_yield(void* context) {
    SubGhzEncoderPrinceton* instance = context;
    if(instance->repeat == 0) return level_duration_reset();

    size_t bit = instance->front / 2;
    bool level = !(instance->front % 2);

    LevelDuration ret;
    if(bit < 24) {
        uint8_t byte = bit / 8;
        uint8_t bit_in_byte = bit % 8;
        bool value = (((uint8_t*)&instance->key)[2 - byte] >> (7 - bit_in_byte)) & 1;
        if(value) {
            ret = level_duration_make(level, level ? instance->te * 3 : instance->te);
        } else {
            ret = level_duration_make(level, level ? instance->te : instance->te * 3);
        }
    } else {
        ret = level_duration_make(level, level ? instance->te : instance->te * 30);
    }

    instance->front++;
    if(instance->front == 50) {
        instance->repeat--;
        instance->front = 0;
    }

    return ret;
}


SubGhzDecoderPrinceton* subghz_decoder_princeton_alloc(void) {
    SubGhzDecoderPrinceton* instance = furi_alloc(sizeof(SubGhzDecoderPrinceton));

    instance->common.name = "Princeton";
    instance->common.code_min_count_bit_for_found = 24;
    instance->common.te_shot = 450; //150;
    instance->common.te_long = 1350; //450;
    instance->common.te_delta = 200; //50;
    instance->common.to_string = (SubGhzProtocolCommonToStr)subghz_decoder_princeton_to_str;
    instance->common.to_save_string =
        (SubGhzProtocolCommonGetStrSave)subghz_decoder_princeton_to_save_str;
    instance->common.to_load_protocol=
        (SubGhzProtocolCommonLoad)subghz_decoder_princeton_to_load_protocol;

    return instance;
}

void subghz_decoder_princeton_free(SubGhzDecoderPrinceton* instance) {
    furi_assert(instance);
    free(instance);
}

/** Send bit 
 * 
 * @param instance - SubGhzDecoderPrinceton instance
 * @param bit - bit
 */
void subghz_decoder_princeton_send_bit(SubGhzDecoderPrinceton* instance, uint8_t bit) {
    if(bit) {
        //send bit 1
        SUBGHZ_TX_PIN_LOW();
        delay_us(instance->common.te_long);
        SUBGHZ_TX_PIN_HIGTH();
        delay_us(instance->common.te_shot);
    } else {
        //send bit 0
        SUBGHZ_TX_PIN_LOW();
        delay_us(instance->common.te_shot);
        SUBGHZ_TX_PIN_HIGTH();
        delay_us(instance->common.te_long);
    }
}

void subghz_decoder_princeton_send_key(
    SubGhzDecoderPrinceton* instance,
    uint64_t key,
    uint8_t bit,
    uint8_t repeat) {
    while(repeat--) {
        SUBGHZ_TX_PIN_LOW();
        //Send start bit
        subghz_decoder_princeton_send_bit(instance, 1);
        //Send header
        delay_us(instance->common.te_shot * 33); //+2 interval v bit 1
        //Send key data
        for(uint8_t i = bit; i > 0; i--) {
            subghz_decoder_princeton_send_bit(instance, bit_read(key, i - 1));
        }
    }
}

void subghz_decoder_princeton_reset(SubGhzDecoderPrinceton* instance) {
    instance->common.parser_step = 0;
}

void subghz_decoder_princeton_parse(
    SubGhzDecoderPrinceton* instance,
    bool level,
    uint32_t duration) {
    switch(instance->common.parser_step) {
    case 0:
        if((!level) && (DURATION_DIFF(duration, instance->common.te_shot * 36) <
                        instance->common.te_delta * 36)) {
            //Found Preambula
            instance->common.parser_step = 1;
            instance->common.code_found = 0;
            instance->common.code_count_bit = 0;
        } else {
            instance->common.parser_step = 0;
        }
        break;
    case 1:
        //save duration
        if(level) {
            instance->common.te_last = duration;
            instance->common.parser_step = 2;
        }
        break;
    case 2:
        if(!level) {
            if(duration >= (instance->common.te_shot * 10 + instance->common.te_delta)) {
                instance->common.parser_step = 1;
                if(instance->common.code_count_bit >=
                   instance->common.code_min_count_bit_for_found) {
                    if(instance->common.code_last_found == instance->common.code_found) {
                        //instance->te = (instance->te+instance->common.te_last)/2; //Option 1 TE averaging
                        if(instance->te > instance->common.te_last)
                            instance->te = instance->common.te_last; //Option 2 TE averaging
                    } else {
                        instance->te = instance->common.te_last;
                    }

                    instance->common.code_last_found = instance->common.code_found;
                    instance->common.code_last_count_bit = instance->common.code_count_bit;
                    instance->common.serial = instance->common.code_found >> 4;
                    instance->common.btn = (uint8_t)instance->common.code_found & 0x00000F;

                    if(instance->common.callback)
                        instance->common.callback(
                            (SubGhzProtocolCommon*)instance, instance->common.context);
                }
                instance->common.code_found = 0;
                instance->common.code_count_bit = 0;
                break;
            }

            if((DURATION_DIFF(instance->common.te_last, instance->common.te_shot) <
                instance->common.te_delta) &&
               (DURATION_DIFF(duration, instance->common.te_long) <
                instance->common.te_delta * 3)) {
                subghz_protocol_common_add_bit(&instance->common, 0);
                instance->common.parser_step = 1;
            } else if(
                (DURATION_DIFF(instance->common.te_last, instance->common.te_long) <
                 instance->common.te_delta * 3) &&
                (DURATION_DIFF(duration, instance->common.te_shot) < instance->common.te_delta)) {
                subghz_protocol_common_add_bit(&instance->common, 1);
                instance->common.parser_step = 1;
            } else {
                instance->common.parser_step = 0;
            }
        } else {
            instance->common.parser_step = 0;
        }
        break;
    }
}

void subghz_decoder_princeton_to_str(SubGhzDecoderPrinceton* instance, string_t output) {
    uint32_t code_found_hi = instance->common.code_last_found >> 32;
    uint32_t code_found_lo = instance->common.code_last_found & 0x00000000ffffffff;

    uint64_t code_found_reverse = subghz_protocol_common_reverse_key(
        instance->common.code_last_found, instance->common.code_last_count_bit);

    uint32_t code_found_reverse_hi = code_found_reverse >> 32;
    uint32_t code_found_reverse_lo = code_found_reverse & 0x00000000ffffffff;

    string_cat_printf(
        output,
        "%s %d Bit te %dus\r\n"
        " KEY:0x%lX%08lX\r\n"
        " YEK:0x%lX%08lX\r\n"
        " SN:0x%05lX BTN:%02X\r\n",
        instance->common.name,
        instance->common.code_last_count_bit,
        instance->te,
        code_found_hi,
        code_found_lo,
        code_found_reverse_hi,
        code_found_reverse_lo,
        instance->common.serial,
        instance->common.btn);
}

void subghz_decoder_princeton_to_save_str(SubGhzDecoderPrinceton* instance, string_t output) {
    string_printf(
        output,
        "Protocol: %s\n"
        "Bit: %d\n"
        "Te: %d\n"
        "Key: %08lX\n",
        instance->common.name,
        instance->common.code_last_count_bit,
        instance->te,
        (uint32_t)(instance->common.code_last_found & 0x00000000ffffffff));
}

bool subghz_decoder_princeton_to_load_protocol(FileWorker* file_worker, SubGhzDecoderPrinceton* instance){
    bool loaded = false;
    string_t temp_str;
    string_init(temp_str);
    int res = 0;
    int data = 0;

    do {
        // Read and parse bit data from 2nd line
        if(!file_worker_read_until(file_worker, temp_str, '\n')) {
            break;
        }
        res = sscanf(string_get_cstr(temp_str), "Bit: %d\n", &data);
        if(res != 1) {
            break;
        }
        instance->common.code_last_count_bit = (uint8_t)data;

        // Read and parse te data from 3nd line
        if(!file_worker_read_until(file_worker, temp_str, '\n')) {
            break;
        }
        res = sscanf(string_get_cstr(temp_str), "Te: %d\n", &data);
        if(res != 1) {
            break;
        }
        instance->te = (uint16_t)data;

        // Read and parse key data from 4nd line
        if(!file_worker_read_until(file_worker, temp_str, '\n')) {
            break;
        }
        uint32_t temp_key = 0;
        res = sscanf(string_get_cstr(temp_str), "Key: %08lX\n", &temp_key);
        if(res != 1) {
            break;
        }
        instance->common.code_last_found = (uint64_t)temp_key;
        instance->common.serial = instance->common.code_last_found >> 4;
        instance->common.btn = (uint8_t)instance->common.code_last_found & 0x00000F;

        loaded = true;
    } while(0);

    string_clear(temp_str);

    return loaded;
}
