# Author: Jaleel Rogers
# Date: 09/22/2024

kernel_module = open('/proc/raptor_maze')
show = kernel_module.readline();
print(show)
kernel_module.close()
