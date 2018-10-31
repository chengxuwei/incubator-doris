// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

package org.apache.doris.analysis;

import org.apache.doris.analysis.ShowAlterStmt.AlterType;
import org.apache.doris.catalog.Catalog;
import org.apache.doris.common.AnalysisException;
import org.apache.doris.common.ErrorCode;
import org.apache.doris.common.ErrorReport;
import org.apache.doris.mysql.privilege.PrivPredicate;
import org.apache.doris.qe.ConnectContext;

/*
 * CANCEL ALTER COLUMN|ROLLUP FROM db_name.table_name
 */
public class CancelAlterTableStmt extends CancelStmt {

    private AlterType alterType;

    private TableName dbTableName;

    public AlterType getAlterType() {
        return alterType;
    }

    public String getDbName() {
        return dbTableName.getDb();
    }

    public String getTableName() {
        return dbTableName.getTbl();
    }

    public CancelAlterTableStmt(AlterType alterType, TableName dbTableName) {
        this.alterType = alterType;
        this.dbTableName = dbTableName;
    }

    @Override
    public void analyze(Analyzer analyzer) throws AnalysisException {
        dbTableName.analyze(analyzer);

        // check access
        if (!Catalog.getCurrentCatalog().getAuth().checkTblPriv(ConnectContext.get(), dbTableName.getDb(),
                                                                dbTableName.getTbl(),
                                                                PrivPredicate.ALTER)) {
            ErrorReport.reportAnalysisException(ErrorCode.ERR_TABLEACCESS_DENIED_ERROR, "CANCEL ALTER TABLE",
                                                ConnectContext.get().getQualifiedUser(),
                                                ConnectContext.get().getRemoteIP(),
                                                dbTableName.getTbl());
        }
    }

    @Override
    public String toSql() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("CANCEL ALTER " + this.alterType);
        stringBuilder.append(" FROM " + dbTableName.toSql());
        return stringBuilder.toString();
    }

    @Override
    public String toString() {
        return toSql();
    }

}