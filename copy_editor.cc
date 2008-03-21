/*
** -- Local Includes --
*/

#include "copy_editor.h"

extern qtbook *qmain;
extern QApplication *qapp;

/*
** -- copy_editor() --
*/

copy_editor::copy_editor(QWidget *parent, qtbook_item *bitemArg,
			 const bool showForLendingArg,
			 const int quantityArg, const QString &ioidArg,
			 const QString &uniqueidArg,
			 QSpinBox *spinboxArg,
			 const QFont &font,
			 const QString &itemTypeArg,
			 const QString &itemTitleArg,
			 const QString &realItemTypeArg): QDialog(parent)
{
  (void) itemTitleArg;

  if(parent == qmain->getMembersBrowser())
    setWindowModality(Qt::ApplicationModal);
  else
    setWindowModality(Qt::WindowModal);

  cb.setupUi(this);
  bitem = bitemArg;
  ioid = ioidArg;
  quantity = quantityArg;
  uniqueid = uniqueidArg;
  spinbox = spinboxArg;
  itemType = itemTypeArg.toLower().remove(" ");
  realItemType = realItemTypeArg;
  showForLending = showForLendingArg;
  cb.table->verticalHeader()->setResizeMode(QHeaderView::Fixed);

  if(!uniqueidArg.isEmpty())
    setWindowTitle(QString("BiblioteQ: Copy Browser (%1)").arg(uniqueidArg));

  setGlobalFonts(font);
}

/*
** -- ~copy_editor() --
*/

copy_editor::~copy_editor()
{
  clearCopiesList();
}

/*
** -- slotDeleteCopy() --
*/

void copy_editor::slotDeleteCopy(void)
{
  int row = cb.table->currentRow();
  bool isCheckedOut = false;
  QString copyid = "";
  QString errorstr = "";

  if(row < 0)
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "Please select a copy to delete.");
      return;
    }
  else if(cb.table->rowCount() == 1)
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "You must have at least one copy.");
      return;
    }

  copyid = misc_functions::getColumnString(cb.table, row, "Barcode");
  qapp->setOverrideCursor(Qt::WaitCursor);
  isCheckedOut = misc_functions::isCopyCheckedOut(qmain->getDB(),
						  copyid,
						  ioid,
						  itemType,
						  errorstr);
  qapp->restoreOverrideCursor();

  if(isCheckedOut)
    {
      if(cb.table->item(row, 0) != NULL)
	cb.table->item(row, 0)->setFlags(0);

      if(cb.table->item(row, 1) != NULL)
	{
	  cb.table->item(row, 1)->setFlags(0);
	  cb.table->item(row, 1)->setText("0");
	}

      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "It appears that the copy you selected to "
			    "delete is reserved.");
      return;
    }
  else if(errorstr.length() > 0)
    {
      qmain->addError(QString("Database Error"),
		      QString("Unable to determine the reservation "
			      "status of the selected copy."),
		      errorstr, __FILE__, __LINE__);
      QMessageBox::critical(this, "BiblioteQ: Database Error",
			    "Unable to determine the reservation status "
			    "of the selected copy.");
      return;
    }

  cb.table->removeRow(cb.table->currentRow());
}

/*
** -- populateCopiesEditor() --
*/

void copy_editor::populateCopiesEditor(void)
{
  int i = 0;
  int j = 0;
  int row = 0;
  QDate duedate = QDate::currentDate().addDays(15);
  QString str = "";
  QSqlQuery query(qmain->getDB());
  QStringList list;
  QTableWidgetItem *item = NULL;

  cb.dueDateFrame->setVisible(showForLending);
  cb.deleteButton->setVisible(!showForLending);

  if(!showForLending)
    {
      cb.saveButton->setText("&Save");
      connect(cb.saveButton, SIGNAL(clicked(void)), this,
	      SLOT(slotSaveCopies(void)));
      connect(cb.deleteButton, SIGNAL(clicked(void)), this,
	      SLOT(slotDeleteCopy(void)));
    }
  else
    {
      /*
      ** Set the minimum date to duedate.
      */

      cb.dueDate->setMinimumDate(duedate);
      cb.saveButton->setText("&Reserve");
      connect(cb.saveButton, SIGNAL(clicked(void)), this,
	      SLOT(slotCheckoutCopy(void)));
    }

  connect(cb.cancelButton, SIGNAL(clicked(void)), this,
	  SLOT(slotCloseCopyEditor(void)));
  QProgressDialog progress1(this);
  QProgressDialog progress2(this);
  cb.table->clear();
  list.append("Barcode");
  list.append("Availability");
  list.append("OID");
  list.append("Copy Number");
  cb.table->setHorizontalHeaderLabels(list);
  list.clear();
  cb.table->setRowCount(quantity);
  cb.table->scrollToTop();
  cb.table->horizontalScrollBar()->setValue(0);

  /*
  ** Hide the Copy Number and OID columns.
  */

  cb.table->setColumnHidden(cb.table->columnCount() - 1, true);
  cb.table->setColumnHidden(cb.table->columnCount() - 2, true);
  updateGeometry();
  resize(sizeHint());
  show();
  progress1.setModal(true);
  progress1.setWindowTitle("BiblioteQ: Progress Dialog");
  progress1.setLabelText("Constructing objects...");
  progress1.setMaximum(quantity);
  progress1.setCancelButton(NULL);
  progress1.show();
  progress1.update();

  for(i = 0; i < quantity; i++)
    {
      for(j = 0; j < cb.table->columnCount(); j++)
	if((item = new QTableWidgetItem()) != NULL)
	  {
	    if(showForLending)
	      item->setFlags(0);
	    else if(j == 0)
	      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
			     Qt::ItemIsEditable);
	    else
	      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	    if(j == 0)
	      {
		if(!uniqueid.isEmpty())
		  str = QString("%1-%2").arg(uniqueid).arg(i + 1);
		else
		  str = QString("CopyIdentifier-%1").arg(i + 1);

		item->setText(str);
	      }
	    else if(j == 1)
	      {
		if(showForLending)
		  item->setText("0");
		else
		  item->setText("1");
	      }
	    else if(j == 2)
	      item->setText(ioid);
	    else
	      item->setText("");

	    cb.table->setItem(i, j, item);
	    cb.table->resizeColumnToContents(j);
	  }
	else
	  qmain->addError(QString("Memory Error"),
			  QString("Unable to allocate memory for the "
				  "\"item\" object. "
				  "This is a serious problem!"),
			  QString(""), __FILE__, __LINE__);

      progress1.setValue(i + 1);
      progress1.update();
      qapp->processEvents();
    }

  cb.table->setRowCount(i);

  if(qmain->getDB().driverName() == "QMYSQL")
    query.prepare(QString("SELECT %1_copy_info.copyid, "
			  "(1 - COUNT(%1_borrower.copyid)), "
			  "%1_copy_info.item_oid, "
			  "%1_copy_info.copy_number "
			  "FROM "
			  "%1_copy_info LEFT JOIN %1_borrower ON "
			  "%1_copy_info.item_oid = "
			  "%1_borrower.item_oid AND "
			  "%1_copy_info.item_oid = "
			  "%1_borrower.item_oid "
			  "WHERE %1_copy_info.item_oid = ? "
			  "GROUP BY %1_copy_info.copyid "
			  "ORDER BY %1_copy_info.copy_number").arg(itemType));
  else
    query.prepare(QString("SELECT %1_copy_info.copyid, "
			  "(1 - COUNT(%1_borrower.copyid)), "
			  "%1_copy_info.item_oid, "
			  "%1_copy_info.copy_number "
			  "FROM "
			  "%1_copy_info LEFT JOIN %1_borrower ON "
			  "%1_copy_info.copyid = "
			  "%1_borrower.copyid AND "
			  "%1_copy_info.item_oid = "
			  "%1_borrower.item_oid "
			  "WHERE %1_copy_info.item_oid = ? "
			  "GROUP BY %1_copy_info.copyid, "
			  "%1_copy_info.item_oid, "
			  "%1_copy_info.copy_number "
			  "ORDER BY %1_copy_info.copy_number").arg(itemType));

  query.bindValue(0, ioid);
  qapp->setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    qmain->addError(QString("Database Error"),
		    QString("Unable to retrieve copy data."),
		    query.lastError().text(), __FILE__, __LINE__);

  qapp->restoreOverrideCursor();
  progress2.setModal(true);
  progress2.setWindowTitle("BiblioteQ: Progress Dialog");
  progress2.setLabelText("Retrieving copy information...");
  progress2.setMaximum(query.size());
  progress2.show();
  progress2.update();
  i = -1;

  while(i++, !progress2.wasCanceled() && query.next())
    {
      if(query.isValid())
	{
	  row = query.value(3).toInt() - 1;

	  for(j = 0; j < cb.table->columnCount(); j++)
	    if(cb.table->item(row, j) != NULL)
	      {
		str = query.value(j).toString();

		if(query.value(1).toString() == "0")
		  cb.table->item(row, j)->setFlags(0);
		else if(showForLending)
		  cb.table->item(row, j)->setFlags
		    (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		else if(j == 0)
		  cb.table->item(row, 0)->setFlags
		    (Qt::ItemIsSelectable | Qt::ItemIsEnabled |
		     Qt::ItemIsEditable);

		if(j == 1)
		  cb.table->item(row, 1)->setText(query.value(1).toString());
		else
		  cb.table->item(row, j)->setText(str);

		cb.table->resizeColumnToContents(j);
	      }
	}

      progress2.setValue(i + 1);
      progress2.update();
      qapp->processEvents();
    }

  query.clear();
  cb.table->resizeColumnsToContents();
}

/*
** -- slotCheckoutCopy() --
*/

void copy_editor::slotCheckoutCopy(void)
{
  int copyrow = cb.table->currentRow();
  int memberrow = qmain->getBB().table->currentRow();
  bool available = false;
  QString name = "";
  QString copyid = "";
  QString duedate = cb.dueDate->date().toString("MM/dd/yyyy");
  QString errorstr = "";
  QString memberid = "";
  QString checkedout = QDate::currentDate().toString("MM/dd/yyyy");
  QString copynumber = "";
  QString availability = "";
  QSqlQuery query(qmain->getDB());

  if(copyrow < 0 || cb.table->item(copyrow, 0) == NULL)
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "Please select a copy to reserve.");
      return;
    }
  else if((cb.table->item(copyrow, 0)->flags() & Qt::ItemIsEnabled) == 0)
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "It appears that the copy you've selected "
			    "is either unavailable or does not exist.");
      return;
    }
  else if(cb.dueDate->date() <= QDate::currentDate())
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "Please select a Due Date that follows "
			    "today's date.");
      return;
    }

  memberid = misc_functions::getColumnString
    (qmain->getBB().table, memberrow, QString("Member ID"));
  copynumber = misc_functions::getColumnString(cb.table, copyrow,
					       QString("Copy Number"));
  copyid = misc_functions::getColumnString(cb.table, copyrow,
					   QString("Barcode"));
  qapp->setOverrideCursor(Qt::WaitCursor);
  available = misc_functions::isCopyAvailable(qmain->getDB(), ioid, copyid,
					      itemType, errorstr);
  qapp->restoreOverrideCursor();

  if(!available && errorstr.length() > 0)
    {
      qmain->addError(QString("Database Error"),
		      QString("Unable to determine the selected copy's "
			      "availability."),
		      errorstr, __FILE__, __LINE__);
      QMessageBox::critical(this, "BiblioteQ: Database Error",
			    "Unable to determine the selected copy's "
			    "availability.");
      return;
    }
  else if(!available)
    {
      QMessageBox::critical(this, "BiblioteQ: User Error",
			    "The copy that you have selected is either "
			    "unavailable or is reserved.");
      return;
    }

  query.prepare(QString("INSERT INTO %1_borrower "
			"(item_oid, "
			"memberid, "
			"reserved_date, "
			"duedate, "
			"copyid, "
			"copy_number, "
			"reserved_by) "
			"VALUES(?, ?, ?, ?, ?, ?, ?)").arg(itemType));
  query.bindValue(0, ioid);
  query.bindValue(1, memberid);
  query.bindValue(2, checkedout);
  query.bindValue(3, duedate);
  query.bindValue(4, copyid);
  query.bindValue(5, copynumber);
  query.bindValue(6, qmain->getAdminID());
  qapp->setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    {
      qapp->restoreOverrideCursor();
      qmain->addError(QString("Database Error"),
		      QString("Unable to create a reserve record."),
		      query.lastError().text(), __FILE__, __LINE__);
      QMessageBox::critical(this, "BiblioteQ: Database Error",
			    "Unable to create a reserve record.");
      return;
    }

  /*
  ** Record the transaction.
  */

  query.prepare(QString("INSERT INTO member_history "
			"(memberid, "
			"item_oid, "
			"copyid, "
			"reserved_date, "
			"duedate, "
			"returned_date, "
			"reserved_by, "
			"type) "
			"VALUES(?, ?, ?, ?, ?, ?, ?, ?)"));
  query.bindValue(0, memberid);
  query.bindValue(1, ioid);
  query.bindValue(2, copyid);
  query.bindValue(3, checkedout);
  query.bindValue(4, duedate);
  query.bindValue(5, QString("N/A"));
  query.bindValue(6, qmain->getAdminID());
  query.bindValue(7, realItemType);

  if(!query.exec())
    qmain->addError(QString("Database Error"),
		    QString("Unable to create a history record."),
		    query.lastError().text(), __FILE__, __LINE__);

  /*
  ** Update the Reserved Items count on the Members Browser.
  */

  qmain->updateMembersBrowser();
  qapp->restoreOverrideCursor();

  /*
  ** Update the availability column.
  */

  availability = misc_functions::getColumnString
    (qmain->getUI().table, bitem->getRow(), "Availability");

  if(availability != "0")
    availability.setNum(availability.toInt() - 1);

  misc_functions::updateColumn
    (qmain->getUI().table, bitem->getRow(), "Availability", availability);

  /*
  ** Update some additional columns.
  */

  if(misc_functions::getColumnString(qmain->getUI().table, bitem->getRow(),
				     "Barcode") == copyid)
    {
      misc_functions::updateColumn
	(qmain->getUI().table, bitem->getRow(), "Due Date",
	 duedate);
      misc_functions::updateColumn
	(qmain->getUI().table, bitem->getRow(), "Reservation Date",
	 checkedout);
      misc_functions::updateColumn
	(qmain->getUI().table, bitem->getRow(), "Member ID",
	 memberid);
      qapp->setOverrideCursor(Qt::WaitCursor);
      name = misc_functions::getMemberName(qmain->getDB(), memberid,
					   errorstr);
      qapp->restoreOverrideCursor();

      if(errorstr.length() > 0)
	qmain->addError(QString("Database Error"),
			QString("Unable to determine the selected copy's "
				"availability."),
			errorstr, __FILE__, __LINE__);

      misc_functions::updateColumn
	(qmain->getUI().table, bitem->getRow(), "Borrower", name);
    }

  slotCloseCopyEditor();
}

/*
** -- slotSaveCopies() --
*/

void copy_editor::slotSaveCopies(void)
{
  int i = 0;
  QString str = "";
  QString errormsg = "";
  QString errorstr = "";
  QString availability = "";
  copy_class *copy = NULL;
  QStringList duplicates;
  QTableWidgetItem *item1 = NULL;
  QTableWidgetItem *item2 = NULL;

  cb.table->setFocus();

  for(i = 0; i < cb.table->rowCount(); i++)
    if(cb.table->item(i, 0) != NULL &&
       cb.table->item(i, 0)->text().trimmed().isEmpty())
      {
	errormsg = QString("Row number %1 contains an empty Barcode.").arg
	  (i + 1);
	QMessageBox::critical(this, "BiblioteQ: User Error", errormsg);
	duplicates.clear();
	return;
      }
    else if(cb.table->item(i, 0) != NULL)
      {
	if(duplicates.contains(cb.table->item(i, 0)->text()))
	  {
	    errormsg = QString("Row number %1 contains a duplicate "
			       "Barcode.").arg(i + 1);
	    QMessageBox::critical(this, "BiblioteQ: User Error", errormsg);
	    duplicates.clear();
	    return;
	  }
	else
	  duplicates.append(cb.table->item(i, 0)->text());
      }

  duplicates.clear();

  while(!copies.isEmpty())
    delete copies.takeFirst();

  qapp->setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().transaction())
    {
      qapp->restoreOverrideCursor();
      qmain->addError(QString("Database Error"),
		      QString("Unable to create a database transaction."),
		      qmain->getDB().lastError().text(), __FILE__, __LINE__);
      QMessageBox::critical(this, "BiblioteQ: Database Error",
			    "Unable to create a database transaction.");
      return;
    }

  qapp->restoreOverrideCursor();

  for(i = 0; i < cb.table->rowCount(); i++)
    {
      item1 = cb.table->item(i, 0);
      item2 = cb.table->item(i, 2);

      if(item1 == NULL || item2 == NULL)
	continue;

      if((copy = new copy_class(item1->text().trimmed(),
				item2->text())) != NULL)
	copies.append(copy);
      else
	qmain->addError
	  (QString("Memory Error"),
	   QString("Unable to allocate memory for the \"copy\" object. "
		   "This is a serious problem!"),
	   QString(""), __FILE__, __LINE__);
    }

  if(saveCopies().isEmpty())
    {
      qapp->setOverrideCursor(Qt::WaitCursor);
      misc_functions::saveQuantity(qmain->getDB(), ioid, copies.size(),
				   itemType, errorstr);
      qapp->restoreOverrideCursor();

      if(!errorstr.isEmpty())
	qmain->addError(QString("Database Error"),
			QString("Unable to save the item's quantity."),
			errorstr, __FILE__, __LINE__);
      else
	goto success_label;
    }

  qapp->setOverrideCursor(Qt::WaitCursor);
  copies.clear();

  if(!qmain->getDB().rollback())
    qmain->addError(QString("Database Error"), QString("Rollback failure."),
		    qmain->getDB().lastError().text(), __FILE__, __LINE__);

  qapp->restoreOverrideCursor();
  QMessageBox::critical(this, "BiblioteQ: Database Error",
			"Unable to save the copy data.");
  return;

 success_label:
  qapp->setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().commit())
    {
      copies.clear();
      qapp->restoreOverrideCursor();
      qmain->addError(QString("Database Error"), QString("Commit failure."),
		      qmain->getDB().lastError().text(), __FILE__, __LINE__);
      QMessageBox::critical(this, "BiblioteQ: Database Error",
			    "Unable to commit the copy data.");
      return;
    }

  qapp->restoreOverrideCursor();
  spinbox->setValue(copies.size());
  qapp->setOverrideCursor(Qt::WaitCursor);
  availability = misc_functions::getAvailability(ioid, qmain->getDB(),
						 itemType, errorstr);
  qapp->restoreOverrideCursor();

  if(!errorstr.isEmpty())
    qmain->addError(QString("Database Error"),
		    QString("Retrieving availability."),
		    errorstr, __FILE__, __LINE__);

  misc_functions::updateColumn(qmain->getUI().table, bitem->getRow(),
			       "Availability", availability);
  str.setNum(copies.size());
  misc_functions::updateColumn(qmain->getUI().table, bitem->getRow(),
			       "Quantity", str);
  copies.clear();
}

/*
** -- saveCopies() --
*/

QString copy_editor::saveCopies(void)
{
  int i = 0;
  QString lastError = "";
  QSqlQuery query(qmain->getDB());
  copy_class *copy = NULL;
  QProgressDialog progress(this);

  query.prepare(QString("DELETE FROM %1_copy_info WHERE "
			"item_oid = ?").arg(itemType));
  query.bindValue(0, ioid);
  qapp->setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    {
      qapp->restoreOverrideCursor();
      qmain->addError(QString("Database Error"),
		      QString("Unable to purge copy data."),
		      query.lastError().text(), __FILE__, __LINE__);
      return query.lastError().text();
    }
  else
    {
      qapp->restoreOverrideCursor();
      progress.setModal(true);
      progress.setWindowTitle("BiblioteQ: Progress Dialog");
      progress.setLabelText("Saving the copy data...");
      progress.setMaximum(copies.size());
      progress.show();
      progress.update();

      for(i = 0; i < copies.size(); i++)
	{
	  copy = copies.at(i);

	  query.prepare(QString("INSERT INTO %1_copy_info "
				"(item_oid, copy_number, "
				"copyid) "
				"VALUES (?, "
				"?, ?)").arg(itemType));
	  query.bindValue(0, copy->itemoid);
	  query.bindValue(1, i + 1);
	  query.bindValue(2, copy->copyid);

	  if(!query.exec())
	    {
	      lastError = query.lastError().text();

	      if(!lastError.toLower().contains("duplicate"))
		qmain->addError(QString("Database Error"),
				QString("Unable to create copy data."),
				query.lastError().text(), __FILE__, __LINE__);
	      else
		lastError = "";
	    }

	  progress.setValue(i + 1);
	  progress.update();
	  qapp->processEvents();
	}
    }

  return lastError;
}

/*
** -- slotCloseCopyEditor() --
*/

void copy_editor::slotCloseCopyEditor(void)
{
  clearCopiesList();
  deleteLater();
}

/*
** -- clearCopiesList() --
*/

void copy_editor::clearCopiesList(void)
{
  while(!copies.isEmpty())
    delete copies.takeFirst();
}

/*
** -- closeEvent() --
*/

void copy_editor::closeEvent(QCloseEvent *e)
{
  (void) e;
  slotCloseCopyEditor();
}

/*
** -- setGlobalFonts() --
*/

void copy_editor::setGlobalFonts(const QFont &font)
{
  setFont(font);

  foreach(QWidget *widget, findChildren<QWidget *>())
    widget->setFont(font);
}
