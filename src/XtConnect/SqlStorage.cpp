/*
    Certain enhancements (www.xtuple.com/trojita-enhancements)
    are copyright © 2010 by OpenMFG LLC, dba xTuple.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    - Neither the name of xTuple nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior
    written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "SqlStorage.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QSqlError>
#include <QVariant>

namespace XtConnect {

SqlStorage::SqlStorage(QObject *parent) :
    QObject(parent)
{
}

void SqlStorage::open()
{
    db = QSqlDatabase::addDatabase( QLatin1String("QPSQL"), QLatin1String("xtconnect-sqlstorage") );
    db.setUserName( "xtrole" );
    db.setDatabaseName("xtbatch");

    if ( ! db.open() ) {
        _fail( "Failed to open database connection", db );
    }

    _prepareStatements();
}

void SqlStorage::_prepareStatements()
{
    _queryInsertMail = QSqlQuery(db);
    if ( ! _queryInsertMail.prepare( QLatin1String("INSERT INTO xtbatch.eml "
                                                   "(eml_hash, eml_date, eml_subj, eml_body, eml_msg, eml_status) "
                                                   "SELECT ?, ?, ?, ?, ?, 'I' WHERE NOT EXISTS "
                                                   " ( SELECT eml_id FROM xtbatch.eml WHERE eml_hash = ? ) "
                                                   "RETURNING eml_id") ) )
        _fail( "Failed to prepare query _queryInsertMail", _queryInsertMail );

    _queryInsertAddress = QSqlQuery(db);
    if ( ! _queryInsertAddress.prepare( QLatin1String("INSERT INTO xtbatch.emladdr "
                                                      "(emladdr_eml_id, emladdr_type, emladdr_addr, emladdr_name) "
                                                      "VALUES (?, ?, ?, ?)") ) )
        _fail( "Failed to prepare query _queryInsertAddress", _queryInsertAddress );

    _queryMarkMailReady = QSqlQuery(db);
    if ( ! _queryMarkMailReady.prepare( QLatin1String("UPDATE xtbatch.eml SET eml_status = 'O' WHERE eml_id = ?") ) )
        _fail( "Failed to prepare query _queryMarkMailReady", _queryMarkMailReady );
}

SqlStorage::ResultType SqlStorage::insertMail( const QDateTime &dateTime, const QString &subject, const QString &readableText, const QByteArray &headers, const QByteArray &body, quint64 &emlId )
{
    QCryptographicHash hash( QCryptographicHash::Sha1 );
    hash.addData( body );
    QByteArray hashValue = hash.result();
    _queryInsertMail.bindValue( 0, hashValue );
    _queryInsertMail.bindValue( 1, dateTime );
    _queryInsertMail.bindValue( 2, subject );
    _queryInsertMail.bindValue( 3, readableText );
    _queryInsertMail.bindValue( 4, headers + body );
    _queryInsertMail.bindValue( 5, hashValue );

    if ( ! _queryInsertMail.exec() ) {
        _fail( "Query _queryInsertMail failed", _queryInsertMail );
        return RESULT_ERROR;
    }

    if ( _queryInsertMail.first() ) {
        emlId = _queryInsertMail.value( 0 ).toULongLong();
        return RESULT_OK;
    } else {
        return RESULT_DUPLICATE;
    }
}

void SqlStorage::_fail( const QString &message, const QSqlQuery &query )
{
    qWarning() << QString::fromAscii("SqlStorage: Query Error: %1: %2").arg( message, query.lastError().text() );
}

void SqlStorage::_fail( const QString &message, const QSqlDatabase &database )
{
    qWarning() << QString::fromAscii("SqlStorage: Query Error: %1: %2").arg( message, database.lastError().text() );
}

void SqlStorage::fail( const QString &message )
{
    _fail( message, db );
}

Common::SqlTransactionAutoAborter SqlStorage::transactionGuard()
{
    return Common::SqlTransactionAutoAborter( &db );
}

SqlStorage::ResultType SqlStorage::insertAddress( const quint64 emlId, const QString &name, const QString &address, const QLatin1String kind )
{
    _queryInsertAddress.bindValue( 0, emlId );
    _queryInsertAddress.bindValue( 1, kind );
    _queryInsertAddress.bindValue( 2, address );
    _queryInsertAddress.bindValue( 3, name );

    if ( ! _queryInsertAddress.exec() ) {
        _fail( "Query _queryInsertAddress failed", _queryInsertAddress );
        return RESULT_ERROR;
    }

    return RESULT_OK;
}

SqlStorage::ResultType SqlStorage::markMailReady( const quint64 emlId )
{
    _queryMarkMailReady.bindValue( 0, emlId );

    if ( ! _queryMarkMailReady.exec() ) {
        _fail( "Query _queryMarkMailReady failed", _queryMarkMailReady );
        return RESULT_ERROR;
    }

    return RESULT_OK;
}

}
