// Pin names for terminal and LCD display, in order
const char *hw_pin_label_ordered[] = {
    "IO0",
    "IO1",
    "IO2",
    "IO3",
    "IO4",
    "IO5",
    "IO6",
    "IO7",
    "GND"
};

const char *func_pin_label_ordered[] = {
    "UTX",
    "URX",
    "TDI",
    "TDO",
    "TCK",
    "TMS",
    "RST",
    "TRST",
    "GND"
};

const char *direction_pin_label_ordered[]={
    "->",
    "<-",
    "->",
    "<-",
    "->",
    "->",
    "->",
    "->",
    "->",
};

static inline void pirate_options_init(void){
    #include "pirate/psu.h"
    #include "pirate/button.h"
    //TODO: set app name, vout label, vout name, vout direction
    const char app_name[] = "DIRTYJTAG";

    if(!button_get(0)){
        const char vout_pin_label[]="VOUT";
        const char vout_func_label[]="3.3V";
        const char vout_direction[]="->";
        ui_lcd_update(
        app_name, 
        vout_pin_label, 
        vout_func_label, 
        vout_direction, 
        hw_pin_label_ordered, 
        func_pin_label_ordered, 
        direction_pin_label_ordered);
        psu_enable(3.3, 0, true);
    }else{
        const char vout_pin_label[]="VREF";
        const char vout_func_label[]="";
        const char vout_direction[]="<-";
        ui_lcd_update(
        app_name, 
        vout_pin_label, 
        vout_func_label, 
        vout_direction, 
        hw_pin_label_ordered, 
        func_pin_label_ordered, 
        direction_pin_label_ordered);
    }
}