#!/usr/bin/env openroad

# Read technology file
read_tech_file testcase1/testcase1.tf

# Read all LEF files
puts "Reading LEF files..."
read_lef testcase1/SNPSHOPT25/lef/snps25hopt.lef
read_lef testcase1/SNPSLOPT25/lef/snps25lopt.lef  
read_lef testcase1/SNPSROPT25/lef/snps25ropt.lef
read_lef testcase1/SNPSSLOPT25/lef/snps25slopt.lef

# Read all Liberty files (using same corner)
puts "Reading Liberty files..."
read_liberty testcase1/SNPSHOPT25/liberty/nldm/base/snps25hopt_base_tt0p8v25c.lib
read_liberty testcase1/SNPSLOPT25/liberty/nldm/base/snps25lopt_base_tt0p8v25c.lib
read_liberty testcase1/SNPSROPT25/liberty/nldm/base/snps25ropt_base_tt0p8v25c.lib
read_liberty testcase1/SNPSSLOPT25/liberty/nldm/base/snps25slopt_base_tt0p8v25c.lib

# Read design
puts "Reading design..."
read_verilog testcase1/testcase1.v
link_design [lindex top 0]

# Read constraints
read_sdc testcase1/testcase1.sdc


puts "Design loaded successfully!"
