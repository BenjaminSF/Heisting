#ifndef _backupManager_
#define _backupManager_

void initBackupQueue();
void addBackupOrder(struct order storeOrder);
void deleteBackupOrder(struct order storeOrder);
void transferBackupOrders();

#endif