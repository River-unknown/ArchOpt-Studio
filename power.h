/*CPU power values*/
#define vdd 1.2 //1.2 V
#define f_clk 1.2e+9 //1.2 GHz
#define cpu_dyn_energy_per_cycle 4.0e-12 //cpu dynamic energy per cycle 4 pJ
#define cpu_lkg_energy_per_cycle 16.0e-12  //cpu leakage energy per cycle 16 pJ
/*memory power values*/
#define dram_energy_per_access 10.5e-9 // main memory dynamic energy per access 10.5 nJ 
#define sram_energy_per_read 1200e-12 // cache memory dynamic energy per read 1200 pJ 
#define sram_energy_per_write 1400e-12 // cache memory dynamic energy per write 1400 pJ
#define sram_lkg_energy_per_cycle 120e-15  //cache memory per bit leakage energy per cycle 120 fJ
#define dram_lkg_energy_per_cycle 20e-15 //main memory per bit leakage energy per cycle 20 fJ
/*mMPU power values*/
#define VDD_mMPU 1.5 // 1.5 V
#define memris_on_energy 1.86875e-12 //1.6e-15 // memristor R_OFF -> R_ON energy  1.6fJ 
#define memris_off_energy 2.0763888888e-13 //1.5e-15 // memristor R_ON -> R_OFF energy 1.5fJ 
/*memory capacitance*/
#define dram_cap  1.0e-12 // dram capacitance 1 pF
#define sram_cap  6.0e-12 // sram capacitance 1 pF
/*bus capacitance*/
#define mem_l2_bus_cap  1.0e-9 // main memory - l2 cache bus capacitance 1 nF
#define l2_dl1_bus_cap   0.25e-9 // l2 cache - l1 dcache bus capacitance 0.25 nF
#define dl1_cpu_bus_cap  0.25e-9 // l1 dcache - CPU bus capacitance 0.25 nF
#define cpu_mmpu_bus_cap  1.0e-9 // CPU - mMPU cache bus capacitance 1 nF
#define inter_mat_bus_cap 0.25e-12 //  inter MAT bus capacitance 0.25 pF
#define inter_bank_bus_cap 0.5e-12 // inter bank bus capacitance 0.5 pF
#define inter_chip_bus_cap 1.0e-12 // inter chip bus capacitance 1 pF
/*flag bit capacitance*/
#define flag_bit_capacitance 1.0e-15
/*alu component capacitance*/
#define comparator_cap 5.0e-11 // comparator capacitance 0.5 pF
#define adder_cap 7.0e-11 // adder capacitance 0.7 pF
#define multiplier_cap 1.2e-12 // multiplier capacitance 1.2 pF
#define divider_cap 1.5e-12 // divider capacitance 1.5 pF
#define mMPU_flag_capacitance 1.0E-15 //1 fF
// mMPU micro operations
#define num_muops 8  
#define muop_set 0
#define muop_reset 1
#define muop_mnot 2
#define muop_mnor 3
#define muop_fill 4
#define muop_write 5
#define muop_jset 6
#define muop_jres 7
//type of memristor bit transitions
#define zero_zero_trans 0
#define zero_one_trans 1
#define one_zero_trans 2
#define one_one_trans 3
//bit transition energy values for muop_set 0
#define muop_set_zero_zero_trans 0.0
#define muop_set_zero_one_trans 3.7275e-12 // 3.7275e-12 J
#define muop_set_one_zero_trans 0.0
#define muop_set_one_one_trans 3.735e-12 // 3.735e-12 J
//bit transition energy values for muop_reset 1
#define muop_reset_zero_zero_trans 1.383333e-15 //1.383333e-15 J
#define muop_reset_zero_one_trans 0.0
#define muop_reset_one_zero_trans 2.248833e-13 //2.248833e-13 J
#define muop_reset_one_one_trans 0.0
//bit transition energy values for muop_mnot 2
#define muop_mnot_zero_zero_trans 0.0
#define muop_mnot_zero_one_trans 0.0
#define muop_mnot_one_zero_trans 6.172757e-13 //6.172757e-13 J 
#define muop_mnot_one_one_trans 5.51495e-15 //5.51495e-15 J
//bit transition energy values for muop_mnor 3
#define muop_mnor_zero_zero_input 1.0993377e-14 //1.0993377e-14 J     o/p 1->1
#define muop_mnor_zero_one_input 6.084235e-13 // 6.084235e-13 J o/p 1->0
#define muop_mnor_one_zero_input 6.084235e-13 // 6.084235e-13 J o/p 1->0
#define muop_mnor_one_one_input  6.9853577e-13 // 6.9853577e-13 J o/p 1->0
//bit transition energy values for muop_fill 4
#define muop_fill_zero_zero_trans 0.0
#define muop_fill_zero_one_trans muop_set_zero_one_trans
#define muop_fill_one_zero_trans 0.0
#define muop_fill_one_one_trans muop_set_one_one_trans
//bit transition energy values for muop_write 5
#define muop_write_zero_zero_trans muop_reset_zero_zero_trans
#define muop_write_zero_one_trans muop_set_zero_one_trans
#define muop_write_one_zero_trans muop_reset_one_zero_trans
#define muop_write_one_one_trans muop_set_one_one_trans
//bit transition energy values for muop_jset 6
#define muop_jset_zero_zero_trans 0.0
#define muop_jset_zero_one_trans 0.0
#define muop_jset_one_zero_trans muop_mnot_one_zero_trans
#define muop_jset_one_one_trans  muop_mnot_one_one_trans
//bit transition energy values for muop_jres 7
#define muop_jres_zero_zero_trans 0.0
#define muop_jres_zero_one_trans 0.0
#define muop_jres_one_zero_trans muop_mnot_one_zero_trans
#define muop_jres_one_one_trans muop_mnot_one_one_trans
//mMPU muop energy per bit
double mMPU_muop_energy_per_bit[num_muops][4]=
  {
    {muop_set_zero_zero_trans,muop_set_zero_one_trans,muop_set_one_zero_trans,muop_set_one_one_trans},          //muop_set 0
    {muop_reset_zero_zero_trans,muop_reset_zero_one_trans,muop_reset_one_zero_trans,muop_reset_one_one_trans},  //muop_reset 1
    {muop_mnot_zero_zero_trans,muop_mnot_zero_one_trans,muop_mnot_one_zero_trans,muop_mnot_one_one_trans},      //muop_mnot 2
    {muop_mnor_zero_zero_input,muop_mnor_zero_one_input,muop_mnor_one_zero_input,muop_mnor_one_one_input},      //muop_mnor 3
    {muop_fill_zero_zero_trans,muop_fill_zero_one_trans,muop_fill_one_zero_trans,muop_fill_one_one_trans},      //muop_fill 4
    {muop_write_zero_zero_trans,muop_write_zero_one_trans,muop_write_one_zero_trans,muop_write_one_one_trans},  //muop_write 5
    {muop_jset_zero_zero_trans,muop_jset_zero_one_trans,muop_jset_one_zero_trans,muop_jset_one_one_trans},      //muop_jset 6
    {muop_jres_zero_zero_trans,muop_jres_zero_one_trans,muop_jres_one_zero_trans,muop_jres_one_one_trans}       //muop_jres 7
  };
// mu-op energy values end
