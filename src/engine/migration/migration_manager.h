#pragma once

#include "migration_types.h"
#include <QObject>
#include <QVariantMap>

namespace MahadevERP {

class MigrationManager : public QObject {
    Q_OBJECT

public:
    explicit MigrationManager(QObject* parent = nullptr);
    ~MigrationManager() override = default;

    static MigrationManager& instance();

    Q_INVOKABLE QVariantMap inspect(MigrationProvider provider, const QString& sourcePath);
    Q_INVOKABLE bool runMigrationPipeline(MigrationContext& ctx);

signals:
    void progressUpdated(int percent, const QString& currentStep);
    void migrationFinished(bool success, const QString& message);

private:
    bool runBahiKhataPipeline(MigrationContext& ctx);
    bool runBusyPipeline(MigrationContext& ctx);
    bool runTallyPipeline(MigrationContext& ctx);
};

} // namespace MahadevERP
