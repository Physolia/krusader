/***************************************************************************
                                 krarc.cpp
                             -------------------
    begin                : Sat Jun 14 14:42:49 IDT 2003
    copyright            : (C) 2003 by Rafi Yanai & Shie Erlich
    email                : krusader@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <kfileitem.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <klocale.h>
#include <kurl.h>
#include <kprocess.h>
#include <ktempfile.h>
#include <klargefile.h>
#include <kstandarddirs.h>
#include <kio/job.h>

#include <iostream>
#include "krarc.h"

//#if 1
#define KRDEBUG(X...) do{   \
	QFile f("/tmp/debug");    \
	f.open(IO_WriteOnly | IO_Append);     \
	QTextStream stream( &f ); \
  stream << X << endl;      \
	f.close();                \
} while(0);
//#else
//#define KRDEBUG(X...)
//#endif

using namespace KIO;
extern "C" {

int kdemain( int argc, char **argv ){
	KInstance instance( "kio_krarc" );

	if (argc != 4){
		kdWarning() << "Usage: kio_krarc  protocol domain-socket1 domain-socket2" << endl;
		exit(-1);
	}
    
	kio_krarcProtocol slave(argv[2], argv[3]);
	slave.dispatchLoop();

	return 0;
}

} // extern "C" 

kio_krarcProtocol::kio_krarcProtocol(const QCString &pool_socket, const QCString &app_socket)
 : SlaveBase("kio_krarc", pool_socket, app_socket), archiveChanged(true), arcFile(0L){
	dirDict.setAutoDelete(true);
	initArcParameters();
 	arcTempDir = locateLocal("tmp",QString::null);
	QString dirName = "krArc"+QDateTime::currentDateTime().toString(Qt::ISODate);
  QDir(arcTempDir).mkdir(dirName);
	arcTempDir = arcTempDir+dirName+"/";
}

/* ---------------------------------------------------------------------------------- */
kio_krarcProtocol::~kio_krarcProtocol(){
	// delete the temp directory
	KShellProcess proc;
	proc << "rm -rf "<< arcTempDir;
	proc.start(KProcess::Block);
}

/* ---------------------------------------------------------------------------------- */
void kio_krarcProtocol::receivedData(KProcess*,char* buf,int len){
	QByteArray d(len);
	d.setRawData(buf,len);
	data(d);	
}

void kio_krarcProtocol::mkdir(const KURL& url,int permissions){
	KRDEBUG("mkdir: "<<url.path());
	if( !setArcFile(url.path()) ){
		error( ERR_DOES_NOT_EXIST,url.path() );
		return;
	}

	QString arcDir  = findArcDirectory(url);
  QString tmpDir = arcTempDir + arcDir.mid(1) + url.path().mid(url.path().findRev("/")+1)+"/";

	if( permissions == -1 ) permissions = 0666; //set default permissions
  for( unsigned int i=arcTempDir.length();i<tmpDir.length(); i=tmpDir.find("/",i+1)){
		::mkdir(tmpDir.left(i).latin1(),permissions);
	}

	// pack the directory
	KShellProcess proc;
	proc << putCmd << "\""+arcFile->url().path()+"\" " << "\""+tmpDir.mid(arcTempDir.length())+"\"";
	infoMessage(i18n("Creating %1 ...").arg( url.fileName() ) );
	QDir::setCurrent(arcTempDir);
	proc.start(KProcess::Block);

  // directory will be deleted in the destructor...

	//  force a refresh of archive information
  initDirDict(url,true);
	finished();
}

void kio_krarcProtocol::put(const KURL& url,int permissions,bool overwrite,bool resume){
	KRDEBUG("put: "<<url.path());
	if( !setArcFile(url.path()) ){
		error( ERR_DOES_NOT_EXIST,url.path() );
		return;
	}
	if( !overwrite && findFileEntry(url) ){
		error( ERR_FILE_ALREADY_EXIST,url.path() );
		return;
	}

	QString arcDir  = findArcDirectory(url);
  QString tmpFile = arcTempDir + arcDir.mid(1) + url.path().mid(url.path().findRev("/")+1);

	QString tmpDir = arcTempDir+arcDir.mid(1)+"/";
  for( unsigned int i=arcTempDir.length();i<tmpDir.length(); i=tmpDir.find("/",i+1)){
		QDir("/").mkdir(tmpDir.left(i));
	}

	int fd;
	if ( resume ) {
		fd = KDE_open( tmpFile.latin1(), O_RDWR );  // append if resuming
		KDE_lseek(fd, 0, SEEK_END); // Seek to end
	} else {
		// WABA: Make sure that we keep writing permissions ourselves,
		// otherwise we can be in for a surprise on NFS.
		mode_t initialMode;
		if ( permissions != -1)
			initialMode = permissions | S_IWUSR | S_IRUSR;
		else
			initialMode = 0666;

		fd = KDE_open(tmpFile.latin1(), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
	}

  QByteArray buffer;
	int readResult;
	do{
		dataReq();
		readResult = readData(buffer);
		write(fd,buffer.data(),buffer.size());
 	} while( readResult > 0 );
	close(fd);

	// pack the file
	KShellProcess proc;
	proc << putCmd << "\""+arcFile->url().path()+"\" " << "\""+tmpFile.mid(arcTempDir.length())+"\"";
	infoMessage(i18n("Packing %1 ...").arg( url.fileName() ) );
	QDir::setCurrent(arcTempDir);
	proc.start(KProcess::Block);

  // remove the file
	KIO::del(tmpFile,false,false);
	
	//  force a refresh of archive information
  initDirDict(url,true);
	finished();
}

void kio_krarcProtocol::get(const KURL& url ){
  KRDEBUG("get: "<<url.path());

	UDSEntry* entry = findFileEntry(url);
	if( !entry ){
		error(KIO::ERR_DOES_NOT_EXIST,url.path());
		return;
	}
	if(KFileItem(*entry,url).isDir()){
  	error(KIO::ERR_IS_DIRECTORY,url.path());
		return;
	}

  QString file = url.path().mid(arcFile->url().path().length()+1);
	KShellProcess proc;
	proc << getCmd << "\""+arcFile->url().path()+"\" " << "\""+file+"\"";
	connect(&proc,SIGNAL(receivedStdout(KProcess*,char*,int)),
           this,SLOT(receivedData(KProcess*,char*,int)) );
  infoMessage(i18n("Unpacking %1 ...").arg( url.fileName() ) );
	proc.start(KProcess::Block,KProcess::AllOutput);
 	data(QByteArray());
  finished();
}

void kio_krarcProtocol::del(KURL const & url, bool isFile){
  KRDEBUG("del: "<<url.path());
	if( !findFileEntry(url) ){
		error(KIO::ERR_DOES_NOT_EXIST,url.path());
		return;
	}
	
	QString file = url.path().mid(arcFile->url().path().length()+1);
	if( !isFile ) file = file + "/";
	KShellProcess proc;
	proc << delCmd << "\""+arcFile->url().path()+"\" " << "\""+file+"\"";
	infoMessage(i18n("Deleting %1 ...").arg( url.fileName() ) );
	proc.start(KProcess::Block);
	//  force a refresh of archive information
  initDirDict(url,true);
	finished();
}

void kio_krarcProtocol::stat( const KURL & url ){
	QString path = url.path();
	KRDEBUG("stat: "<<path);
	KURL newUrl = url;
	
  // but treat the archive itself as the archive root
	if( path == arcFile->url().path() ){
    newUrl.setPath(path+"/");
		path = newUrl.path();
		KRDEBUG("newUrl: "<<path);
	}

	// we might be stating a real file
	if( QFileInfo(path).exists() ){
		struct stat buff;
    ::stat( path.latin1(), &buff );
		QString mime = KMimeType::findByPath(path,buff.st_mode)->name();
		KRDEBUG("Stating real file: "<<path);  
		statEntry(KFileItem(path,mime,buff.st_mode).entry());
		finished();
		return;
	}

 	UDSEntry* entry = findFileEntry(newUrl);
	if( entry ){
		statEntry( *entry );
		finished();
	}
	else error(KIO::ERR_DOES_NOT_EXIST,path);
}

void kio_krarcProtocol::listDir(const KURL& url){
	QString path = url.path();
	if( path.right(1) != "/" ) path = path+"/";
	KRDEBUG("listDir: "<<path);	

	// it might be a real dir !
	if( QFileInfo(path).exists() ){
    if( QFileInfo(path).isDir() ){
			KURL redir;
			redir.setPath( url.path() );
			KRDEBUG( "redirection: "<<redir.prettyURL() );
    	redirection(redir);
			finished();
		}
		else { // maybe it's an zip archive !
			error(ERR_IS_FILE,path);
		}
		return;
	}

  if( !initDirDict(url) ){
		error(ERR_CANNOT_ENTER_DIRECTORY,url.path());
		return;
	}
 	QString arcDir = path.mid(arcFile->url().path().length());
	arcDir.truncate(arcDir.findRev("/"));
	if(arcDir.right(1) != "/") arcDir = arcDir+"/";

	UDSEntryList* dirList = dirDict.find(arcDir);
	if( dirList == 0 ){
		error(ERR_CANNOT_ENTER_DIRECTORY,url.path());
		return;
	}
	totalSize(dirList->size());
  listEntries(*dirList);
	finished();
}

bool kio_krarcProtocol::setArcFile(const QString& path){
	archiveChanged = true;
	// is the file already set ?
	if( arcFile && arcFile->url().path() == path.left(arcFile->url().path().length()) ){
		// Has it changed ?
		KFileItem* newArcFile = new KFileItem(arcFile->url(),QString::null,0);
		if( newArcFile->time(UDS_MODIFICATION_TIME) != arcFile->time(UDS_MODIFICATION_TIME) ){
      delete arcFile;
			arcFile = newArcFile;
		}
		else { // same old file
			delete newArcFile;
			archiveChanged = false;
		}
	}
	else {// it's a new file...
		if( arcFile ){
			delete arcFile;
			arcFile = 0L;
		}
		QString newPath = path;
		if(newPath.right(1) != "/") newPath = newPath+"/";
		for(int pos=0; pos >= 0; pos = newPath.find("/",pos+1)){
			QFileInfo qfi(newPath.left(pos));
			if( qfi.exists() && !qfi.isDir() ){
    		arcFile = new KFileItem(newPath.left(pos),QString::null,0);
				break;
			}
		}
		if( !arcFile ) return false; // file not found
	}
 	return true;
}

bool kio_krarcProtocol::initDirDict(const KURL& url, bool forced){
	// set the archive location
	if( !setArcFile(url.path()) ) return false;
	// no need to rescan the archive if it's not changed
	if( !archiveChanged && !forced ) return true;
	// write the temp file
	KShellProcess proc;
	KTempFile temp("krarc","tmp");
	temp.setAutoDelete(true);

	proc << listCmd << "\""+arcFile->url().path()+"\"" <<" > " << temp.name();
	proc.start(KProcess::Block);
	if( !proc.normalExit() || !proc.exitStatus() == 0 )	return false;

	// clear the dir dictionary
	dirDict.clear();

	// add the "/" directory
	UDSEntryList* root = new UDSEntryList();
	dirDict.insert("/",root);
	// and the "/" UDSEntry
	UDSEntry entry;
	UDSAtom atom;
	atom.m_uds = UDS_NAME;
	atom.m_str = ".";
	entry.append(atom);
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = S_IFDIR;
	entry.append(atom);
	root->append(entry);

	// parse the temp file
	temp.file()->open(IO_ReadOnly);
	char buf[1000];
	QString line;

	int lineNo = 0;
	while(temp.file()->readLine(buf,1000) != -1){
		line = QString::fromLocal8Bit(buf);
		parseLine(lineNo++,line.stripWhiteSpace(),temp.file());
	}
  // close and delete our file
	temp.file()->close();

	archiveChanged = false;
	return true;
}

QString kio_krarcProtocol::findArcDirectory(const KURL& url){
	QString path = url.path();
	if( path.right(1) == "/" ) path.truncate(path.length()-1);

  if( !initDirDict(url) ){
		return QString::null;
	}
 	QString arcDir = path.mid(arcFile->url().path().length());
	arcDir.truncate(arcDir.findRev("/"));
	if(arcDir.right(1) != "/") arcDir = arcDir+"/";

	return arcDir;
}

UDSEntry* kio_krarcProtocol::findFileEntry(const KURL& url){
  QString arcDir = findArcDirectory(url);
	if( arcDir.isEmpty() ) return 0;

	UDSEntryList* dirList = dirDict.find(arcDir);
	if( !dirList ){
		return 0;
	}
	QString name = url.path();
	if( name.right(1) == "/" && arcDir == "/" ) name = "."; // the "/" case
	else{
		if( name.right(1) == "/" ) name.truncate(name.length()-1);
		name = name.mid(name.findRev("/")+1);
	}

  UDSEntryList::iterator entry;
	UDSEntry::iterator atom;

	for ( entry = dirList->begin(); entry != dirList->end(); ++entry ){
    for( atom = (*entry).begin(); atom != (*entry).end(); ++atom ){
      if( (*atom).m_uds == UDS_NAME ){
				if((*atom).m_str == name){
          return &(*entry);
				}
				else break;
			}
		}
	}
	return 0;

}

QString kio_krarcProtocol::nextWord(QString &s,char d) {
  s=s.stripWhiteSpace();
  int j=s.find(d,0);
  QString temp=s.left(j); // find the leftmost word.
  s.remove(0,j);
  return temp;
}

mode_t kio_krarcProtocol::parsePermString(QString perm){
	mode_t mode=0;
  // file type
	if(perm[0] == 'd') mode |= S_IFDIR;
	if(perm[0] == 'l') mode |= S_IFLNK;
	if(perm[0] == '-') mode |= S_IFREG;
	// owner permissions
	if(perm[1] != '-') mode |= S_IRUSR;
	if(perm[2] != '-') mode |= S_IWUSR;
	if(perm[3] != '-') mode |= S_IXUSR;
	// group permissions
	if(perm[4] != '-') mode |= S_IRGRP;
	if(perm[5] != '-') mode |= S_IWGRP;
	if(perm[6] != '-') mode |= S_IXGRP;
	// other permissions
	if(perm[7] != '-') mode |= S_IROTH;
	if(perm[8] != '-') mode |= S_IWOTH;
	if(perm[9] != '-') mode |= S_IXOTH;

	return mode;
}

UDSEntryList* kio_krarcProtocol::addNewDir(QString path){
  UDSEntryList* dir;

  // check if the current dir exists
  dir = dirDict.find(path);
  if(dir != 0) return dir; // dir exists- return it !

  // set dir to the parent dir
  KRDEBUG("addNewDir: "<<path);
  dir = addNewDir(path.left(path.findRev("/",-2)+1));

  // do we have an entry for the parent dir ?
  QString name = path.mid(path.findRev("/",-2)+1);
  name = name.left(name.length()-1);
  UDSEntryList::iterator entry;
  bool dirEntryExists = false;
	for ( entry = dir->begin(); entry != dir->end() && !dirEntryExists; ++entry ){
    UDSEntry::iterator atom;
    for( atom = (*entry).begin(); atom != (*entry).end(); ++atom ){
      if( (*atom).m_uds == UDS_NAME ){
				if((*atom).m_str == name){
          dirEntryExists = true;
				}
				else break;
			}
		}
	}
  // add a new entry in the parent dir
  if( !dirEntryExists ){
	  UDSEntry entry;
	  UDSAtom atom;
	  atom.m_uds = UDS_NAME;
	  atom.m_str = name;
	  entry.append(atom);
	  atom.m_uds = UDS_FILE_TYPE;
	  atom.m_long = S_IFDIR;
	  entry.append(atom);
    atom.m_uds = UDS_MODIFICATION_TIME;
    atom.m_long = arcFile->time(UDS_MODIFICATION_TIME);
	  entry.append( atom );
    dir->append(entry);
  } 
  // create a new directory entry and add it..
  dir = new UDSEntryList();
  dirDict.insert(path,dir);

  return dir;
}

void kio_krarcProtocol::parseLine(int, QString line, QFile*){
  UDSEntryList* dir;
	UDSEntry entry;
	UDSAtom atom;

	// permissions
	QString perm = nextWord(line);
	if(perm.length() != 10) perm = (perm.at(0)=='d')? "drwxr-xr-x" : "-rw-r--r--" ;
  mode_t mode = parsePermString(perm);
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = mode & S_IFMT; // keep file type only
	entry.append( atom );
	atom.m_uds = UDS_ACCESS;
	atom.m_long = mode & 07777; // keep permissions only
	entry.append( atom );
 	// ignore the next 2 fields
	nextWord(line); nextWord(line);
  // size
	long size = nextWord(line).toLong();
	atom.m_uds = UDS_SIZE;
  atom.m_long = size;
  entry.append( atom );
 	// ignore the next 2 fields
	nextWord(line);nextWord(line);
	// date & time
	QString d = nextWord(line);
	QDate date(d.mid(0,4).toInt(),d.mid(4,2).toInt(),d.mid(6,2).toInt());
  QTime time(d.mid(9,2).toInt(),d.mid(11,2).toInt(),d.mid(13,2).toInt());
	atom.m_uds = UDS_MODIFICATION_TIME;
	atom.m_long = QDateTime(date,time).toTime_t();
	entry.append( atom );
	// name
	QString fullName = nextWord(line,'\n');
	if( fullName.right(1) == "/" ) fullName = fullName.left(fullName.length()-1);
	if( !fullName.startsWith("/") ) fullName = "/"+fullName;
	QString path = fullName.left(fullName.findRev("/")+1);
	// set/create the directory UDSEntryList
  dir = dirDict.find(path);
  if(dir == 0) dir = addNewDir(path);
	QString name = fullName.mid(fullName.findRev("/")+1);
	atom.m_uds = UDS_NAME;
	atom.m_str = name;
	entry.append(atom);
	dir->append(entry);
}

void kio_krarcProtocol::initArcParameters(){
	cmd = "zip";
	listCmd = "unzip -ZTs-z-t-h ";
	getCmd  = "unzip -p ";
	delCmd  = "zip -d ";
	putCmd  = "zip -ry ";
}
