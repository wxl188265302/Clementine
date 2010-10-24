/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "playlistgenerator.h"
#include "playlistgeneratorinserter.h"
#include "core/taskmanager.h"
#include "playlist/playlist.h"

#include <QFutureWatcher>
#include <QtConcurrentRun>

typedef QFuture<PlaylistItemList> Future;
typedef QFutureWatcher<PlaylistItemList> FutureWatcher;

PlaylistGeneratorInserter::PlaylistGeneratorInserter(
    TaskManager* task_manager, LibraryBackend* library, QObject* parent)
  : QObject(parent),
    task_manager_(task_manager),
    library_(library),
    task_id_(-1)
{
}

static PlaylistItemList Generate(PlaylistGeneratorPtr generator) {
  return generator->Generate();
}

void PlaylistGeneratorInserter::Load(
    Playlist* destination, int row, bool play_now, PlaylistGeneratorPtr generator) {
  task_id_ = task_manager_->StartTask(tr("Loading smart playlist"));

  destination_ = destination;
  row_ = row;
  play_now_ = play_now;

  connect(generator.get(), SIGNAL(Error(QString)), SIGNAL(Error(QString)));

  Future future = QtConcurrent::run(Generate, generator);
  FutureWatcher* watcher = new FutureWatcher(this);
  watcher->setFuture(future);

  connect(watcher, SIGNAL(finished()), SLOT(Finished()));
}

void PlaylistGeneratorInserter::Finished() {
  FutureWatcher* watcher = static_cast<FutureWatcher*>(sender());
  watcher->deleteLater();

  PlaylistItemList items = watcher->result();

  QModelIndex index = destination_->InsertItems(items, row_);
  if (play_now_)
    emit PlayRequested(index);

  task_manager_->SetTaskFinished(task_id_);

  deleteLater();
}
