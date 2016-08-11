#ifndef _MPU6050_H
#define _MPU6050_H

/* registermap from
 https://store.invensense.com/Datasheets/invensense/RM-MPU-6000A.pdf
 */

#define MPU6050_ADDRESS 0x68
#define MPU6050_PWRMNGR_1 0x6b
#define MPU6050_AX 0x3b
#define MPU6050_AY 0x3d
#define MPU6050_AZ 0x3f
#define MPU6050_GX 0x43
#define MPU6050_GY 0x45
#define MPU6050_GZ 0x47
#define MPU6050_TMP 0x41

int mpu6050Setup();
short mpu6050GetAx(int fd);
short mpu6050GetAy(int fd);
short mpu6050GetAz(int fd);
short mpu6050GetGx(int fd);
short mpu6050GetGy(int fd);
short mpu6050GetGz(int fd);
short mpu6050GetTmp(int fd);
#endif
