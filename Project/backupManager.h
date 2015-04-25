#ifndef _backupManager_
#define _backupManager_

void initBackupQueue();
void addBackupOrder(int floor, int button, int elevator);
void deleteBackupOrder(int floor, int button, int elevator);
void transferBackupOrders();

#endif