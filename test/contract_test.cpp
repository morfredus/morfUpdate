/*
 * morfUpdate - contrat d'etats & reconciliation self-update (stage 1)
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Tests du contrat fige AVANT d'ecrire l'applieur destructeur (voir
 * Evolution/morfUpdate - auto-mise a jour (succession de processus).md).
 * Assertions natives, sans framework : le natif suffit.
 */

#include "morfupdate/OperationStore.h"
#include "morfupdate/ReleaseValidator.h"
#include "morfupdate/RequestGuard.h"
#include "morfupdate/Version.h"
#include "morfupdate/UpdateOperation.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTemporaryDir>

#include <cstdio>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

using morfupdate::UpdateState;
using morfupdate::UpdateOperation;
using morfupdate::OperationStore;

// Ecrit un journal operations.json dans `dir` a partir d'entrees brutes.
void writeJournal(const QString& dir, const QJsonArray& operations) {
    QDir().mkpath(dir);
    QSaveFile file(QDir(dir).filePath(QStringLiteral("operations.json")));
    CHECK(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("operations"), operations}})
                   .toJson(QJsonDocument::Indented));
    CHECK(file.commit());
}

QJsonObject rawOp(const QString& id, const QString& state, bool selfUpdate,
                  const QString& from, const QString& to) {
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QJsonObject o{
        {QStringLiteral("id"), id},
        {QStringLiteral("project"), QStringLiteral("morfUpdate")},
        {QStringLiteral("from_version"), from},
        {QStringLiteral("to_version"), to},
        {QStringLiteral("platform"), QStringLiteral("linux-arm64")},
        {QStringLiteral("state"), state},
        {QStringLiteral("detail"), QString()},
        {QStringLiteral("created_at"), now},
        {QStringLiteral("updated_at"), now},
    };
    if (selfUpdate)
        o.insert(QStringLiteral("self_update"), true);
    return o;
}

// -- A. Contrat d'etats ----------------------------------------------------
void testStateContract() {
    // Round-trip nom <-> etat pour les nouveaux etats (valeurs de fil stables).
    CHECK(morfupdate::updateStateName(UpdateState::RollbackPrepared) == QStringLiteral("rollback_prepared"));
    CHECK(morfupdate::updateStateName(UpdateState::Delegated) == QStringLiteral("delegated"));
    CHECK(morfupdate::updateStateName(UpdateState::RolledBack) == QStringLiteral("rolled_back"));

    // Finalite.
    CHECK(morfupdate::isFinal(UpdateState::RolledBack));
    CHECK(!morfupdate::isFinal(UpdateState::Delegated));
    CHECK(!morfupdate::isFinal(UpdateState::RollbackPrepared));

    // Chemin self-update autorise.
    CHECK(morfupdate::canTransition(UpdateState::Verifying, UpdateState::RollbackPrepared));
    CHECK(morfupdate::canTransition(UpdateState::RollbackPrepared, UpdateState::Delegated));
    CHECK(morfupdate::canTransition(UpdateState::Delegated, UpdateState::Succeeded));
    CHECK(morfupdate::canTransition(UpdateState::Delegated, UpdateState::RolledBack));
    CHECK(morfupdate::canTransition(UpdateState::RollbackPrepared, UpdateState::RolledBack));

    // Flux normal preserve.
    CHECK(morfupdate::canTransition(UpdateState::Verifying, UpdateState::Installing));
    CHECK(morfupdate::canTransition(UpdateState::HealthCheck, UpdateState::Succeeded));

    // Transitions illegales.
    CHECK(!morfupdate::canTransition(UpdateState::Delegated, UpdateState::HealthCheck));
    CHECK(!morfupdate::canTransition(UpdateState::Installing, UpdateState::RollbackPrepared));
    CHECK(!morfupdate::canTransition(UpdateState::RolledBack, UpdateState::Succeeded)); // depuis un final
    CHECK(!morfupdate::canTransition(UpdateState::HealthCheck, UpdateState::RolledBack));
}

// -- B. Fonction pure de reconciliation ------------------------------------
void testReconcileFunction() {
    UpdateOperation op;
    op.fromVersion = QStringLiteral("0.6.0");
    op.toVersion = QStringLiteral("0.7.0");

    CHECK(morfupdate::reconcileSelfUpdate(op, true, QStringLiteral("0.7.0")) == UpdateState::Succeeded);
    CHECK(morfupdate::reconcileSelfUpdate(op, true, QStringLiteral("0.6.0")) == UpdateState::RolledBack);
    CHECK(morfupdate::reconcileSelfUpdate(op, true, QStringLiteral("9.9.9")) == UpdateState::Failed);
    // Jamais un succes si le service n'est pas sain, meme a la bonne version.
    CHECK(morfupdate::reconcileSelfUpdate(op, false, QStringLiteral("0.7.0")) == UpdateState::Failed);
}

// -- C. Integration OperationStore : load() + reconcileSelfUpdates ----------
void testSelfUpdateSucceeded() {
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    writeJournal(tmp.path(), {rawOp(QStringLiteral("op-succeed"), QStringLiteral("delegated"),
                                    true, QStringLiteral("0.6.0"), QStringLiteral("0.7.0"))});
    OperationStore store(tmp.path());
    CHECK(store.load());
    // load() ne DOIT PAS avoir fait echouer un self-update non final.
    auto before = store.find(QStringLiteral("op-succeed"));
    CHECK(before.has_value());
    CHECK(before->state == UpdateState::Delegated);
    // Le successeur tourne la version cible -> Succeeded.
    CHECK(store.reconcileSelfUpdates(QStringLiteral("0.7.0")) == 1);
    auto after = store.find(QStringLiteral("op-succeed"));
    CHECK(after.has_value());
    CHECK(after->state == UpdateState::Succeeded);
}

void testSelfUpdateRolledBack() {
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    writeJournal(tmp.path(), {rawOp(QStringLiteral("op-rollback"), QStringLiteral("delegated"),
                                    true, QStringLiteral("0.6.0"), QStringLiteral("0.7.0"))});
    OperationStore store(tmp.path());
    CHECK(store.load());
    // Le successeur est l'ancien binaire restaure par l'applieur -> RolledBack.
    CHECK(store.reconcileSelfUpdates(QStringLiteral("0.6.0")) == 1);
    auto after = store.find(QStringLiteral("op-rollback"));
    CHECK(after.has_value());
    CHECK(after->state == UpdateState::RolledBack);
}

void testNonSelfInterruptionStillFails() {
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    // Mise a jour d'un AUTRE service, interrompue : doit rester un echec franc.
    writeJournal(tmp.path(), {rawOp(QStringLiteral("op-other"), QStringLiteral("installing"),
                                    false, QStringLiteral("1.0.0"), QStringLiteral("1.1.0"))});
    OperationStore store(tmp.path());
    CHECK(store.load());
    auto after = store.find(QStringLiteral("op-other"));
    CHECK(after.has_value());
    CHECK(after->state == UpdateState::Failed);
    // Rien a reconcilier cote self-update.
    CHECK(store.reconcileSelfUpdates(QStringLiteral("0.7.0")) == 0);
}

// -- D. Revue C++ 2026-11-07 : fonctions pures de securite -------------------
void testSecurityGuards() {
    using namespace morfupdate;
    // Anti-CSRF : Origin different de Host sur une ecriture = refus.
    CHECK(!isCrossOriginWrite("POST", "", "127.0.0.1:8794"));
    CHECK(!isCrossOriginWrite("POST", "http://127.0.0.1:8794", "127.0.0.1:8794"));
    CHECK(isCrossOriginWrite("POST", "http://evil.example", "127.0.0.1:8794"));
    CHECK(isCrossOriginWrite("POST", "null", "127.0.0.1:8794"));
    CHECK(!isCrossOriginWrite("GET", "http://evil.example", "127.0.0.1:8794"));
    // DNS rebinding : seul un Host de boucle locale passe.
    CHECK(isLocalHost("127.0.0.1:8794"));
    CHECK(isLocalHost("localhost:8794"));
    CHECK(isLocalHost("LOCALHOST"));
    CHECK(isLocalHost(""));
    CHECK(!isLocalHost("evil.example:8794"));
    CHECK(!isLocalHost("127.0.0.1.evil.example"));
    CHECK(!isLocalHost("127.0.0.1:abc"));
    // En-tetes.
    CHECK(headerValue("GET / HTTP/1.1\r\nhost: a:1\r\nOrigin: http://b", "Host") == "a:1");
    CHECK(headerValue("GET / HTTP/1.1\r\nhost: a:1", "Origin").isEmpty());
    // URL de sante : l'hote reel doit etre 127.0.0.1 (pas de contournement par userinfo).
    CHECK(isLoopbackHealthUrl(QStringLiteral("http://127.0.0.1:8790/healthz")));
    CHECK(!isLoopbackHealthUrl(QStringLiteral("http://127.0.0.1:80@evil.example/")));
    CHECK(!isLoopbackHealthUrl(QStringLiteral("http://evil.example:80/")));
    CHECK(!isLoopbackHealthUrl(QStringLiteral("https://127.0.0.1:8790/healthz")));
    CHECK(!isLoopbackHealthUrl(QStringLiteral("http://user@127.0.0.1:8790/healthz")));
    CHECK(!isLoopbackHealthUrl(QStringLiteral("http://127.0.0.1/healthz")));
    // Downgrade.
    CHECK(isNotNewer(QStringLiteral("1.2.0"), QStringLiteral("1.1.9")));
    CHECK(isNotNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.0")));
    CHECK(!isNotNewer(QStringLiteral("1.2.0"), QStringLiteral("1.2.1")));
    CHECK(!isNotNewer(QString(), QStringLiteral("1.0.0")));      // illisible : pas de blocage a l'aveugle
    // Archives : liens et peripheriques refuses.
    CHECK(!ReleaseValidator::listingHasSpecialEntry(
        QStringLiteral("drwxr-xr-x u/g 0 d/") + QChar('\n') + QStringLiteral("-rw-r--r-- u/g 3 d/f")));
    CHECK(ReleaseValidator::listingHasSpecialEntry(
        QStringLiteral("lrwxrwxrwx u/g 0 d/l -> /etc/passwd")));
    CHECK(ReleaseValidator::listingHasSpecialEntry(QStringLiteral("hrw-r--r-- u/g 0 d/h link to d/f")));
}

// Une entree illisible du journal ne bloque pas le demarrage.
void testInvalidJournalEntryIgnored() {
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    writeJournal(tmp.path(), {QJsonObject{{QStringLiteral("id"), QStringLiteral("broken")}},
                              rawOp(QStringLiteral("good"), QStringLiteral("succeeded"), false,
                                    QStringLiteral("1.0.0"), QStringLiteral("1.1.0"))});
    OperationStore store(tmp.path());
    CHECK(store.load());
    CHECK(!store.find(QStringLiteral("broken")).has_value());
    CHECK(store.find(QStringLiteral("good")).has_value());
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testStateContract();
    testReconcileFunction();
    testSelfUpdateSucceeded();
    testSelfUpdateRolledBack();
    testNonSelfInterruptionStillFails();
    testSecurityGuards();
    testInvalidJournalEntryIgnored();
    if (g_failures == 0) {
        std::printf("morfUpdate contract test: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "morfUpdate contract test: %d check(s) failed\n", g_failures);
    return 1;
}
