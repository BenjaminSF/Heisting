#ifndef _backupManager_
#define _backupManager_

void initBackupQueue();
void addBackupOrder(struct Order storeOrder);
void deleteBackupOrder(struct Order storeOrder);
void transferBackupOrders();

#endif