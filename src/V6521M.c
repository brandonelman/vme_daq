#include "V6521M.h"

uint32_t hv_data;
int32_t handle;

CVErrorCodes write_to_v6521m(uint32_t vme_addr, uint32_t data){
  CVDataWidth data_size = cvD16;
  CVAddressModifier addr_mode = cvA32_U_DATA;
  vme_addr = vme_addr + V6521M_BASE_ADDRESS;
  return CAENVME_WriteCycle(handle, vme_addr, &data, addr_mode, data_size);     
}

CVErrorCodes read_from_v6521m(uint32_t vme_addr){
  CVDataWidth data_size = cvD16;
  CVAddressModifier addr_mode = cvA32_U_DATA;
  vme_addr = vme_addr + V6521M_BASE_ADDRESS;
  return CAENVME_ReadCycle(handle, vme_addr, &hv_data, addr_mode, data_size);
}

CVErrorCodes set_svmax(uint32_t ch_num, uint32_t vmax){

  uint32_t vme_addr;
  vmax = (uint32_t) vmax/VOLTAGE_RESOLUTION; /*e.g. 3 kV => 30000 in register*/
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_SVMAX_CH0;
             break;
    case 1:  vme_addr = V6521M_SVMAX_CH1;
             break;
    case 2:  vme_addr = V6521M_SVMAX_CH2;
             break;
    case 3:  vme_addr = V6521M_SVMAX_CH3;
             break;
    case 4:  vme_addr = V6521M_SVMAX_CH4;
             break;
    case 5:  vme_addr = V6521M_SVMAX_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, vmax);
}

CVErrorCodes set_current(uint32_t ch_num, uint32_t current){
  uint32_t vme_addr;
  current = (uint32_t) current/CURRENT_RESOLUTION; /*e.g. 100microA => 
                                             20000 in register*/
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_ISET_CH0;
             break;
    case 1:  vme_addr = V6521M_ISET_CH1;
             break;
    case 2:  vme_addr = V6521M_ISET_CH2;
             break;
    case 3:  vme_addr = V6521M_ISET_CH3;
             break;
    case 4:  vme_addr = V6521M_ISET_CH4;
             break;
    case 5:  vme_addr = V6521M_ISET_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, current);
}

CVErrorCodes set_voltage(uint32_t ch_num, uint32_t voltage){
  uint32_t vme_addr;
  voltage = (uint32_t) voltage/VOLTAGE_RESOLUTION; 
                                          
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_VSET_CH0;
             break;
    case 1:  vme_addr = V6521M_VSET_CH1;
             break;
    case 2:  vme_addr = V6521M_VSET_CH2;
             break;
    case 3:  vme_addr = V6521M_VSET_CH3;
             break;
    case 4:  vme_addr = V6521M_VSET_CH4;
             break;
    case 5:  vme_addr = V6521M_VSET_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, voltage);
}

int get_voltage(uint32_t ch_num){
  uint32_t vme_addr;
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_VMON_CH0;
             break;
    case 1:  vme_addr = V6521M_VMON_CH1;
             break;
    case 2:  vme_addr = V6521M_VMON_CH2;
             break;
    case 3:  vme_addr = V6521M_VMON_CH3;
             break;
    case 4:  vme_addr = V6521M_VMON_CH4;
             break;
    case 5:  vme_addr = V6521M_VMON_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  read_from_v6521m(vme_addr);
  return (int) hv_data*VOLTAGE_RESOLUTION;
}

int get_current(uint32_t ch_num){
  uint32_t vme_addr;
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_IMONH_CH0;
             break;
    case 1:  vme_addr = V6521M_IMONH_CH1;
             break;
    case 2:  vme_addr = V6521M_IMONH_CH2;
             break;
    case 3:  vme_addr = V6521M_IMONH_CH3;
             break;
    case 4:  vme_addr = V6521M_IMONH_CH4;
             break;
    case 5:  vme_addr = V6521M_IMONH_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  read_from_v6521m(vme_addr);
  return (int) hv_data*CURRENT_RESOLUTION;
}

CVErrorCodes set_ramp_down(uint32_t ch_num, uint32_t volts_per_sec){
  uint32_t vme_addr;
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_RAMP_DOWN_CH0;
             break;
    case 1:  vme_addr = V6521M_RAMP_DOWN_CH1;
             break;
    case 2:  vme_addr = V6521M_RAMP_DOWN_CH2;
             break;
    case 3:  vme_addr = V6521M_RAMP_DOWN_CH3;
             break;
    case 4:  vme_addr = V6521M_RAMP_DOWN_CH4;
             break;
    case 5:  vme_addr = V6521M_RAMP_DOWN_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, volts_per_sec);
}

CVErrorCodes set_ramp_up(uint32_t ch_num, uint32_t volts_per_sec){
  uint32_t vme_addr;
  switch(ch_num)
  {
    case 0:  vme_addr = V6521M_RAMP_UP_CH0;
             break;
    case 1:  vme_addr = V6521M_RAMP_UP_CH1;
             break;
    case 2:  vme_addr = V6521M_RAMP_UP_CH2;
             break;
    case 3:  vme_addr = V6521M_RAMP_UP_CH3;
             break;
    case 4:  vme_addr = V6521M_RAMP_UP_CH4;
             break;
    case 5:  vme_addr = V6521M_RAMP_UP_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, volts_per_sec);
}

CVErrorCodes set_trip_time(uint32_t ch_num, uint32_t trip_time){
  uint32_t vme_addr;
  trip_time = (uint32_t) trip_time/TIME_RESOLUTION;

  switch(ch_num) {
    case 0:  vme_addr = V6521M_TRIP_TIME_CH0;
             break;
    case 1:  vme_addr = V6521M_TRIP_TIME_CH1;
             break;
    case 2:  vme_addr = V6521M_TRIP_TIME_CH2;
             break;
    case 3:  vme_addr = V6521M_TRIP_TIME_CH3;
             break;
    case 4:  vme_addr = V6521M_TRIP_TIME_CH4;
             break;
    case 5:  vme_addr = V6521M_TRIP_TIME_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, trip_time);
}

CVErrorCodes enable_channel(uint32_t ch_num){
  uint32_t vme_addr;

  switch(ch_num) {
    case 0:  vme_addr = V6521M_PW_CH0;
             break;
    case 1:  vme_addr = V6521M_PW_CH1;
             break;
    case 2:  vme_addr = V6521M_PW_CH2;
             break;
    case 3:  vme_addr = V6521M_PW_CH3;
             break;
    case 4:  vme_addr = V6521M_PW_CH4;
             break;
    case 5:  vme_addr = V6521M_PW_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, 1);
}

CVErrorCodes disable_channel(uint32_t ch_num){
  uint32_t vme_addr;

  switch(ch_num) {
    case 0:  vme_addr = V6521M_PW_CH0;
             break;
    case 1:  vme_addr = V6521M_PW_CH1;
             break;
    case 2:  vme_addr = V6521M_PW_CH2;
             break;
    case 3:  vme_addr = V6521M_PW_CH3;
             break;
    case 4:  vme_addr = V6521M_PW_CH4;
             break;
    case 5:  vme_addr = V6521M_PW_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, 0);
}

CVErrorCodes ramp_down_channel(uint32_t ch_num){
  uint32_t vme_addr;

  switch(ch_num) {
    case 0:  vme_addr = V6521M_PWDOWN_CH0;
             break;
    case 1:  vme_addr = V6521M_PWDOWN_CH1;
             break;
    case 2:  vme_addr = V6521M_PWDOWN_CH2;
             break;
    case 3:  vme_addr = V6521M_PWDOWN_CH3;
             break;
    case 4:  vme_addr = V6521M_PWDOWN_CH4;
             break;
    case 5:  vme_addr = V6521M_PWDOWN_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, 1);
}

CVErrorCodes kill_channel(uint32_t ch_num){
  uint32_t vme_addr;

  switch(ch_num) {
    case 0:  vme_addr = V6521M_PWDOWN_CH0;
             break;
    case 1:  vme_addr = V6521M_PWDOWN_CH1;
             break;
    case 2:  vme_addr = V6521M_PWDOWN_CH2;
             break;
    case 3:  vme_addr = V6521M_PWDOWN_CH3;
             break;
    case 4:  vme_addr = V6521M_PWDOWN_CH4;
             break;
    case 5:  vme_addr = V6521M_PWDOWN_CH5;
             break;
    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }
  return write_to_v6521m(vme_addr, 0);
}

