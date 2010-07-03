#include "fakedrivesettings.h"

FakeDriveSettings::FakeDriveSettings(QString prefixName, QWidget * parent, Qt::WFlags f) : QDialog(parent, f)
{
    // Loading libq4wine-core.so
    libq4wine.setFileName("libq4wine-core");

    if (!libq4wine.load()){
        libq4wine.load();
    }

    // Getting corelib calss pointer
    CoreLibClassPointer = (CoreLibPrototype *) libq4wine.resolve("createCoreLib");
    CoreLib.reset((corelib *)CoreLibClassPointer(true));

    this->prefixName=prefixName;

    setupUi(this);

    splitter.reset(new QSplitter(widgetContent));
    splitter->addWidget(optionsTreeWidget);
    splitter->addWidget(optionsStack);

    QList<int> size;
    size << 150 << 150;

    splitter->setSizes(size);

    std::auto_ptr<QVBoxLayout> vlayout (new QVBoxLayout);
    vlayout->addWidget(splitter.release());
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    widgetContent->setLayout(vlayout.release());

    setWindowTitle(tr("Fake drive settings"));
    lblCaption->setText(tr("Fake drive settings for prefix \"%1\"").arg(this->prefixName));

    connect(optionsTree, SIGNAL(itemClicked (QTreeWidgetItem *, int)), this, SLOT(optionsTree_itemClicked ( QTreeWidgetItem *, int)));

    connect(cmdCancel, SIGNAL(clicked()), this, SLOT(cmdCancel_Click()));
    QString pic="", line="", prefixPath="";
    connect(cmdOk, SIGNAL(clicked()), this, SLOT(cmdOk_Click()));
    connect(cmdHelp, SIGNAL(clicked()), this, SLOT(cmdHelp_Click()));

    loadThemeIcons();

    QList<QTreeWidgetItem *> items = optionsTree->findItems (tr("General"), Qt::MatchExactly);
    if (items.count()>0){
        items.at(0)->setExpanded(true);
        optionsTree->setCurrentItem(items.at(0));
        optionsStack->setCurrentIndex(0);
        tabwGeneral->setCurrentIndex(0);
    }

    cmdGetWineDesktop->installEventFilter(this);
    cmdGetWineDesktopDoc->installEventFilter(this);
    cmdGetWineDesktopPic->installEventFilter(this);
    cmdGetWineDesktopMus->installEventFilter(this);
    cmdGetWineDesktopVid->installEventFilter(this);

}

void FakeDriveSettings::optionsTree_itemClicked ( QTreeWidgetItem *item, int){
    if (!item)
        return;

    item->setExpanded(true);

    QString itemText = item->text(0);

    if (itemText==tr("General")){
        optionsStack->setCurrentIndex(0);
        tabwGeneral->setCurrentIndex(0);
    } else if (itemText==tr("Color theme")){
        optionsStack->setCurrentIndex(0);
        tabwGeneral->setCurrentIndex(1);
    } else if (itemText==tr("Wine browsers")){
        optionsStack->setCurrentIndex(0);
        tabwGeneral->setCurrentIndex(2);
    } else if (itemText==tr("Video")){
        optionsStack->setCurrentIndex(1);
        tabwVideo->setCurrentIndex(0);
    } else if (itemText==tr("Direct 3D") && item->parent()->text(0)==tr("Video")){
        optionsStack->setCurrentIndex(1);
        tabwVideo->setCurrentIndex(1);
    } else if (itemText==tr("OpenGL")){
        optionsStack->setCurrentIndex(1);
        tabwVideo->setCurrentIndex(2);
    } else if (itemText==tr("X11 driver") && item->parent()->text(0)==tr("Video")){
        optionsStack->setCurrentIndex(1);
        tabwVideo->setCurrentIndex(3);
    } else if (itemText==tr("File system")){
        optionsStack->setCurrentIndex(3);
        tabwFileSystem->setCurrentIndex(0);
    } else if (itemText==tr("Wine drives")){
        optionsStack->setCurrentIndex(3);
        tabwFileSystem->setCurrentIndex(0);
    } else if (itemText==tr("Desktop paths")){
        optionsStack->setCurrentIndex(3);
        tabwFileSystem->setCurrentIndex(1);
    } else if (itemText==tr("Audio")){
        optionsStack->setCurrentIndex(2);
        tabwAudio->setCurrentIndex(0);
    } else if (itemText==tr("Sound driver")){
        optionsStack->setCurrentIndex(2);
        tabwAudio->setCurrentIndex(0);
    } else if (itemText==tr("Alsa driver")){
        optionsStack->setCurrentIndex(2);
        tabwAudio->setCurrentIndex(1);
    } else if (itemText==tr("Misc audio")){
        optionsStack->setCurrentIndex(2);
        tabwAudio->setCurrentIndex(2);
    } else if (itemText==tr("Input")){
        optionsStack->setCurrentIndex(4);
        tabwInput->setCurrentIndex(0);
    } else if (itemText==tr("Direct 3D") && item->parent()->text(0)==tr("Input")){
        optionsStack->setCurrentIndex(4);
        tabwInput->setCurrentIndex(0);
    } else if (itemText==tr("X11 driver") && item->parent()->text(0)==tr("Input")){
        optionsStack->setCurrentIndex(4);
        tabwInput->setCurrentIndex(1);
    }
}

void FakeDriveSettings::loadPrefixSettings(){
    this->loadSettings();
    return;
}

void FakeDriveSettings::loadDefaultPrefixSettings(){
    this->loadDefaultSettings();
    return;
}

void FakeDriveSettings::cmdOk_Click(){
    QRegExp rx("^\".*\"=\".*\"$");
    QList<QListWidgetItem *> listItems = listJoystickAxesMappings->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);

    for (int i=0; i < listItems.count(); i++){
        if (rx.indexIn(listItems.at(i)->text())!=0){
            QMessageBox::warning(this, tr("Error"), tr("Error in string:\n\n%1\n\nJoystick axes mappings might be defined as:\n\"Joystick name\"=\"axes mapping\"\n\nFor example:\n\"Logitech Logitech Dual Action\"=\"X,Y,Rz,Slider1,POV1\"\n\nSee help for details.").arg(listItems.at(i)->text()));
            return;
        }
    }

    if (listWineDrives->count()>0){
        QString tmppath=QDir::homePath();
        bool tmpexists=FALSE;
        tmppath.append("/.config/q4wine/tmp");
        for (int i=0; i<listWineDrives->count(); i++){
            QString path = listWineDrives->item(i)->text().split("\n").at(0).split(":").at(1).trimmed();

            if (path==tmppath){
                tmpexists=TRUE;
                break;
            }
        }
        if (!tmpexists){
            QMessageBox::warning(this, tr("Warning"), tr("Can't find Wine Drive which is point to:\n\"%1\"\n\nMake shure wine can access %2 temp directory.").arg(tmppath).arg(APP_SHORT_NAME));
        }
    }

    QApplication::setOverrideCursor( Qt::BusyCursor );
    this->setEnabled(false);

    //Getting versions
    QString version;
    if (comboFakeVersion->currentText()=="Windows XP"){
        version = "winxp";
    } else if (comboFakeVersion->currentText()=="Windows 2008"){
        version = "win2008";
    } else if (comboFakeVersion->currentText()=="Windows 7"){
        version = "win7";
    } else  if (comboFakeVersion->currentText()=="Windows Vista"){
        version = "vista";
    } else if (comboFakeVersion->currentText()=="Windows 2003"){
        version = "win2003";
    } else if (comboFakeVersion->currentText()=="Windows 2000"){
        version = "win2k";
    } else if (comboFakeVersion->currentText()=="Windows ME"){
        version = "winme";
    } else if (comboFakeVersion->currentText()=="Windows 98"){
        version = "win98";
    } else if (comboFakeVersion->currentText()=="Windows 95"){
        version = "win95";
    } else if (comboFakeVersion->currentText()=="Windows NT 4.0"){
        version = "nt40";
    } else if (comboFakeVersion->currentText()=="Windows NT 3.0"){
        version = "nt351";
    } else if (comboFakeVersion->currentText()=="Windows 3.1"){
        version = "win31";
    } else if (comboFakeVersion->currentText()=="Windows 3.0"){
        version = "win30";
    } else if (comboFakeVersion->currentText()=="Windows 2.0"){
        version = "win20";
    }

    ExecObject execObj;
    execObj.cmdargs = "-u -i";
    execObj.execcmd = "wineboot";

    if (!CoreLib->runWineBinary(execObj, prefixName, false)){
        QApplication::restoreOverrideCursor();
        reject();
        return;
    }

    Registry reg(db_prefix.getPath(prefixName));
    QStringList list;
    list << "\"Desktop\""<<"\"My Music\""<<"\"My Pictures\""<<"\"My Videos\""<<"\"Personal\"";
    list = reg.readKeys("user", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", list);

    if (list.count()==5){
        desktopFolder = CoreLib->decodeRegString(list.at(0).split("\\\\").last());
        desktopDocuments = CoreLib->decodeRegString(list.at(1).split("\\\\").last());
        desktopMusic = CoreLib->decodeRegString(list.at(2).split("\\\\").last());
        desktopPictures = CoreLib->decodeRegString(list.at(3).split("\\\\").last());
        desktopVideos = CoreLib->decodeRegString(list.at(4).split("\\\\").last());
    } else {
         QMessageBox::warning(this, tr("Error"), tr("Can't read desktop paths!"));
        this->reject();
        return;
    }

     qDebug()<<desktopFolder<<desktopDocuments<<desktopMusic<<desktopPictures<<desktopVideos;

    QString sh_cmd = "";
    QStringList sh_line;

    QDir wineDriveDir;
    wineDriveDir.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot  );

    QString prefixPath = db_prefix.getPath(prefixName);
    prefixPath.append("/dosdevices/");

    if (!wineDriveDir.cd(prefixPath)){
        qDebug()<<"Cannot cd to prefix directory: "<<prefixPath;
    } else {
        QFileInfoList drivelist = wineDriveDir.entryInfoList();
        for (int i = 0; i < drivelist.size(); ++i) {
            QFileInfo fileInfo = drivelist.at(i);
            if (fileInfo.isSymLink()){
                sh_cmd.clear();
                sh_cmd.append(CoreLib->getWhichOut("rm"));
                sh_cmd.append(" -fr '");
                sh_cmd.append(fileInfo.filePath());
                sh_cmd.append("'");
                sh_line.append(sh_cmd);
            }
        }
    }

    sh_cmd.clear();
    sh_cmd.append("cd \'");
    sh_cmd.append(prefixPath);
    sh_cmd.append("\'");
    sh_line.append(sh_cmd);

    if (listWineDrives->count()>0){
        for (int i=0; i<listWineDrives->count(); i++){
            std::auto_ptr<DriveListWidgetItem> item (dynamic_cast<DriveListWidgetItem*>(listWineDrives->item(i)));

            if (item.get()){
                sh_cmd.clear();
                sh_cmd.append(CoreLib->getWhichOut("rm"));
                sh_cmd.append(" -f '");
                sh_cmd.append(item->getLetter().toLower());
                sh_cmd.append("'");
                sh_line.append(sh_cmd);

                sh_cmd.clear();
                sh_cmd.append(CoreLib->getWhichOut("ln"));
                sh_cmd.append(" -s '");
                sh_cmd.append(item->getPath());
                sh_cmd.append("' '");
                sh_cmd.append(item->getLetter().toLower());
                sh_cmd.append("'");
                sh_line.append(sh_cmd);
            }
            item.release();
        }
    }

    prefixPath.append("c:/users/");
    prefixPath.append(getenv("USER"));

    sh_cmd.clear();
    sh_cmd.append("mkdir -p \'");
    sh_cmd.append(prefixPath);
    sh_cmd.append("\'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("rm"));
    sh_cmd.append(" -fr '");
    sh_cmd.append(QString("%1/%2").arg(prefixPath).arg(desktopFolder));
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("rm"));
    sh_cmd.append(" -fr '");
    sh_cmd.append(QString("%1/%2").arg(prefixPath).arg(desktopDocuments));
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("rm"));
    sh_cmd.append(" -fr '");
    sh_cmd.append(QString("%1/%2").arg(prefixPath).arg(desktopMusic));
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("rm"));
    sh_cmd.append(" -fr '");
    sh_cmd.append(QString("%1/%2").arg(prefixPath).arg(desktopPictures));
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("rm"));
    sh_cmd.append(" -fr '");
    sh_cmd.append(QString("%1/%2").arg(prefixPath).arg(desktopVideos));
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append("cd \'");
    sh_cmd.append(prefixPath);
    sh_cmd.append("\'");
    sh_line.append(sh_cmd);

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("ln"));
    sh_cmd.append(" -s '");
    sh_cmd.append(txtWineDesktop->text());
    sh_cmd.append("' '");
    sh_cmd.append(desktopFolder);
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    QDir dir (txtWineDesktop->text());
    if (!dir.exists())
        if (!dir.mkpath(dir.path())){
        QMessageBox::warning(this, tr("Error"), tr("Can't create dir: %1").arg(dir.path()));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::can't create dir: "<<dir.path();
#endif
    }

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("ln"));
    sh_cmd.append(" -s '");
    sh_cmd.append(txtWineDesktopDoc->text());
    sh_cmd.append("' '");
    sh_cmd.append(desktopDocuments);
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    dir.setPath (txtWineDesktopDoc->text());
    if (!dir.exists())
        if (!dir.mkpath(dir.path())){
        QMessageBox::warning(this, tr("Error"), tr("Can't create dir: %1").arg(dir.path()));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::can't create dir: "<<dir.path();
#endif
    }

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("ln"));
    sh_cmd.append(" -s '");
    sh_cmd.append(txtWineDesktopMus->text());
    sh_cmd.append("' '");
    sh_cmd.append(desktopMusic);
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    dir.setPath (txtWineDesktopMus->text());
    if (!dir.exists())
        if (!dir.mkpath(dir.path())){
        QMessageBox::warning(this, tr("Error"), tr("Can't create dir: %1").arg(dir.path()));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::can't create dir: "<<dir.path();
#endif
    }

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("ln"));
    sh_cmd.append(" -s '");
    sh_cmd.append(txtWineDesktopPic->text());
    sh_cmd.append("' '");
    sh_cmd.append(desktopPictures);
    sh_cmd.append("'");
    sh_line.append(sh_cmd);

    dir.setPath (txtWineDesktopPic->text());
    if (!dir.exists())
        if (!dir.mkpath(dir.path())){
        QMessageBox::warning(this, tr("Error"), tr("Can't create dir: %1").arg(dir.path()));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::can't create dir: "<<dir.path();
#endif
    }

    sh_cmd.clear();
    sh_cmd.append(CoreLib->getWhichOut("ln"));
    sh_cmd.append(" -s '");
    sh_cmd.append(txtWineDesktopVid->text());
    sh_cmd.append("' '");
    sh_cmd.append(desktopVideos);
    sh_cmd.append("'");
    sh_line.append(sh_cmd);


    dir.setPath(txtWineDesktopVid->text());
    if (!dir.exists())
        if (!dir.mkpath(dir.path())){
        QMessageBox::warning(this, tr("Error"), tr("Can't create dir: %1").arg(dir.path()));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::can't create dir: "<<dir.path();
#endif
    }

    sh_cmd.clear();

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::Creating sh_line";
#endif
    for (int i=0; i<sh_line.count(); i++){
        sh_cmd.append(sh_line.at(i));
        if (i!=(sh_line.count()-1))
            sh_cmd.append(" && ");
    }

    QStringList args;
    args<<"-c"<<sh_cmd;

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::Updateing wine dosdrives";
#endif


    Process proc(args, CoreLib->getWhichOut("sh"), QDir::homePath(), tr("Updateing wine dosdrives"), tr("Updateing wine dosdrives"), true);
    proc.exec();

    // ---- End of Creating Dos drives ----

    Registry registry;

    if (!registry.init()){
        QApplication::restoreOverrideCursor();
        this->reject();
        return;
    }
    registry.set("Software\\Microsoft\\Windows NT\\CurrentVersion", "RegisteredOrganization", txtOrganization->text(), "HKEY_LOCAL_MACHINE");
    registry.set("Software\\Microsoft\\Windows NT\\CurrentVersion", "RegisteredOwner", txtOwner->text(), "HKEY_LOCAL_MACHINE");

    registry.set("Software\\Wine", "Version", version);


    if (cbCrashDialog->isChecked()){
        registry.set("Software\\Wine\\WineDbg", "ShowCrashDialog", "dword:00000000");
    } else {
        registry.unset("Software\\Wine\\WineDbg", "ShowCrashDialog");
    }

    if (listWineDrives->count()>0){

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::Create wine drive config";
#endif

    for (int i=0; i<listWineDrives->count(); i++){
        std::auto_ptr<DriveListWidgetItem> item (dynamic_cast<DriveListWidgetItem*>(listWineDrives->item(i)));
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::get DriveListWidgetItem for "<<listWineDrives->item(i)->text();
#endif
        if (item.get()){
            if (item->getType()=="auto"){
                registry.unset("Software\\Wine\\Drives", item->getLetter().toLower(), "HKEY_LOCAL_MACHINE");
            } else {
                registry.set("Software\\Wine\\Drives", "", QString("\"%1\"=\"%2\"").arg(item->getLetter().toLower()).arg(item->getType()), "HKEY_LOCAL_MACHINE");
            }
        }
        item.release();
    }
}

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::creating registry cfg";
#endif

    if (!txtFakeBrowsers->text().isEmpty()){
        registry.set("Software\\Wine\\WineBrowser", "Browsers", txtFakeBrowsers->text());
    } else {
        registry.unset("Software\\Wine\\WineBrowser", "Browsers");
    }

    if (!txtFakeMailers->text().isEmpty()){
        registry.set("Software\\Wine\\WineBrowser", "Mailers", txtFakeMailers->text());
    } else {
        registry.unset("Software\\Wine\\WineBrowser", "Mailers");
    }

    if (comboFakeD3D_Multi->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "Multisampling", comboFakeD3D_Multi->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "Multisampling");
    }

    if (comboFakeD3D_Render->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "DirectDrawRenderer", comboFakeD3D_Render->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "DirectDrawRenderer");
    }

    if (comboFakeD3D_LMode->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "RenderTargetLockMode", comboFakeD3D_LMode->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "RenderTargetLockMode");
    }

    if (comboFakeD3D_SDOrder->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "StrictDrawOrdering", comboFakeD3D_SDOrder->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "StrictDrawOrdering");
    }

    if (comboFakeD3D_Offscreen->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "OffscreenRenderingMode", comboFakeD3D_Offscreen->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "OffscreenRenderingMode");
    }

    if (comboFakeD3D_Offscreen->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "OffscreenRenderingMode", comboFakeD3D_Offscreen->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "OffscreenRenderingMode");
    }

    if (comboFakeD3D_GLSL->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "UseGLSL", comboFakeD3D_GLSL->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "UseGLSL");
    }

    if (comboFakeD3D_N2M->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "Nonpower2Mode", comboFakeD3D_N2M->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "Nonpower2Mode");
    }

    if (!txtFakeVideoMemory->text().isEmpty()){
        registry.set("Software\\Wine\\Direct3D", "VideoMemorySize", txtFakeVideoMemory->text());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "VideoMemorySize");
    }

    if (!txtFakeVideoDescription->text().isEmpty()){
        registry.set("Software\\Wine\\Direct3D", "VideoDescription", txtFakeVideoDescription->text());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "VideoDescription");
    }

    if (!txtFakeVideoDriver->text().isEmpty()){
        registry.set("Software\\Wine\\Direct3D", "VideoDriver", txtFakeVideoDriver->text());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "VideoDriver");
    }

    if (comboFakeSoftwareEmulation->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "SoftwareEmulation", comboFakeSoftwareEmulation->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "SoftwareEmulation");
    }

    if (comboFakePixelShaderMode->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "PixelShaderMode", comboFakePixelShaderMode->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "PixelShaderMode");
    }

    if (comboFakeVertexShaderMode->currentText()!="default"){
        registry.set("Software\\Wine\\Direct3D", "VertexShaderMode", comboFakeVertexShaderMode->currentText());
    } else {
        registry.unset("Software\\Wine\\Direct3D", "VertexShaderMode");
    }

    if (!txtFakeDisabledExtensions->text().isEmpty()){
        registry.set("Software\\Wine\\OpenGL", "DisabledExtensions", txtFakeDisabledExtensions->text());
    } else {
        registry.unset("Software\\Wine\\OpenGL", "DisabledExtensions");
    }

    if (sboxFakeInput_scroll->value()>0){
        registry.set("Control Panel\\Desktop", "WheelScrollLines", QString("%1").arg(sboxFakeInput_scroll->value()));
    } else {
        registry.unset("Control Panel\\Desktop", "WheelScrollLines");
    }

    if (comboFakeInput_selection->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "UsePrimarySelection", comboFakeInput_selection->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "UsePrimarySelection");
    }

    registry.unsetPath("Software\\Wine\\DirectInput");

    if (listJoystickAxesMappings->count()>0){
#ifdef DEBUG
        qDebug()<<"[ii] Wizard::creating registry cfg for joystik";
#endif

        for (int i=0; i<listJoystickAxesMappings->count(); i++){
            registry.set("Software\\Wine\\DirectInput", "", listJoystickAxesMappings->item(i)->text());
        }
    }

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::creating registry cfg";
#endif


    if (comboFakeMouseWarp->currentText()!="default"){
        registry.set("Software\\Wine\\DirectInput", "MouseWarpOverride", comboFakeMouseWarp->currentText());
    } else {
        registry.unset("Software\\Wine\\DirectInput", "MouseWarpOverride");
    }

    if (comboFakeX11_WR->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "ClientSideWithRender", comboFakeX11_WR->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "ClientSideWithRender");
    }

    if (comboFakeX11_AAR->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "ClientSideAntiAliasWithRender", comboFakeX11_AAR->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "ClientSideAntiAliasWithRender");
    }

    if (comboFakeX11_AAC->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "ClientSideAntiAliasWithCore", comboFakeX11_AAC->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "ClientSideAntiAliasWithCore");
    }

    if (comboFakeX11_XRandr->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "UseXRandR", comboFakeX11_XRandr->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "UseXRandR");
    }

    if (comboFakeX11_XVid->currentText()!="default"){
        registry.set("Software\\Wine\\X11 Driver", "UseXVidMode", comboFakeX11_XVid->currentText());
    } else {
        registry.unset("Software\\Wine\\X11 Driver", "UseXVidMode");
    }

    if (comboFakeSound_Driver->currentText()!="default"){
        if (comboFakeSound_Driver->currentText()=="disabled"){
            registry.set("Software\\Wine\\Drivers", "Audio", "");
        } else {
            registry.set("Software\\Wine\\Drivers", "Audio", comboFakeSound_Driver->currentText());
        }
    } else {
        registry.unset("Software\\Wine\\Drivers", "Audio");
    }

    if (comboFakeAlsa_asCards->currentText()!="default"){
        registry.set("Software\\Wine\\Alsa Driver", "AutoScanCards", comboFakeAlsa_asCards->currentText());
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "AutoScanCards");
    }

    if (comboFakeAlsa_asDevices->currentText()!="default"){
        registry.set("Software\\Wine\\Alsa Driver", "AutoScanDevices", comboFakeAlsa_asDevices->currentText());
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "AutoScanDevices");
    }

    if (sboxFakeAlsa_devCount->value()>0){
        registry.set("Software\\Wine\\Alsa Driver", "DeviceCount", QString("%1").arg(sboxFakeAlsa_devCount->value()));
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "DeviceCount");
    }

    if (!txtFakeAlsa_CTLn->text().isEmpty()){
        registry.set("Software\\Wine\\Alsa Driver", "DeviceCTLn", txtFakeAlsa_CTLn->text());
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "DeviceCTLn");
    }

    if (!txtFakeAlsa_PCMn->text().isEmpty()){
        registry.set("Software\\Wine\\Alsa Driver", "DevicePCMn", txtFakeAlsa_PCMn->text());
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "DevicePCMn");
    }

    if (comboFakeAlsa_DirectHW->currentText()!="default"){
        registry.set("Software\\Wine\\Alsa Driver", "UseDirectHW", comboFakeAlsa_DirectHW->currentText());
    } else {
        registry.unset("Software\\Wine\\Alsa Driver", "UseDirectHW");
    }

    if (sboxFakeSound_shadow->value()>-1){
        registry.set("Software\\Wine\\DirectSound", "MaxShadowSize", QString("%1").arg(sboxFakeSound_shadow->value()));
    } else {
        registry.unset("Software\\Wine\\DirectSound", "MaxShadowSize");
    }

    if (rbColorsDefault->isChecked()){
        registry.unsetPath("Control Panel\\Colors");
    }

    if (rbColorsQt4->isChecked()){

#ifdef DEBUG
        qDebug()<<"[ii] Wizard::creating registry cfg for color";
#endif

        QColor color;
        QPalette cur_palette;

        cur_palette = qApp->palette();
        color = cur_palette.color(QPalette::Base);
        registry.set("Control Panel\\Colors", "Window", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::Window);
        registry.set("Control Panel\\Colors", "ActiveBorder", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "InactiveBorder", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "AppWorkSpace", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "Menu", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "MenuBar", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "Scrollbar", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "MenuHilight", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "ButtonFace", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::AlternateBase);
        registry.set("Control Panel\\Colors", "ButtonAlternateFace", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::Dark);
        registry.set("Control Panel\\Colors", "ButtonDkShadow", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "ButtonShadow", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "GrayText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::Light);
        registry.set("Control Panel\\Colors", "ButtonHilight", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::ButtonText);
        registry.set("Control Panel\\Colors", "ButtonText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::WindowText);
        registry.set("Control Panel\\Colors", "MenuText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "WindowFrame", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
        registry.set("Control Panel\\Colors", "WindowText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::Highlight);
        registry.set("Control Panel\\Colors", "Hilight", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::HighlightedText);
        registry.set("Control Panel\\Colors", "HilightText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::ToolTipBase);
        registry.set("Control Panel\\Colors", "InfoWindow", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));

        color = cur_palette.color(QPalette::ToolTipText);
        registry.set("Control Panel\\Colors", "InfoText", QString("%1 %2 %3").arg(QString::number(color.red())) .arg(QString::number(color.green())) .arg(QString::number(color.blue())));
    }

#ifdef DEBUG
    qDebug()<<"[ii] Wizard::run registry import";
#endif

    if (registry.exec(this, prefixName)){
        CoreLib->createPrefixDBStructure(prefixName);
#ifdef DEBUG
    qDebug()<<"[ii] Wizard::done";
#endif
    QTimer::singleShot(3000, this, SLOT(waitForWineEnd()));
        } else {
            QApplication::restoreOverrideCursor();
            reject();
        }
    return;
}

void FakeDriveSettings::waitForWineEnd(){
    QApplication::restoreOverrideCursor();
    this->accept();
}

void FakeDriveSettings::cmdCancel_Click(){
    this->reject();
    return;
}

void FakeDriveSettings::cmdHelp_Click(){
    return;
}

void FakeDriveSettings::loadThemeIcons(){
    lblLogo->setPixmap(CoreLib->loadPixmap("data/exec.png"));
    connect(cmdJoystickEdit, SIGNAL(clicked()), this, SLOT(cmdJoystickEdit_Click()));
    connect(cmdJoystickAdd, SIGNAL(clicked()), this, SLOT(cmdJoystickAdd_Click()));
    connect(cmdJoystickDel, SIGNAL(clicked()), this, SLOT(cmdJoystickDel_Click()));
    connect(cmdWineDriveEdit, SIGNAL(clicked()), this, SLOT(cmdWineDriveEdit_Click()));
    connect(cmdWineDriveAdd, SIGNAL(clicked()), this, SLOT(cmdWineDriveAdd_Click()));
    connect(cmdWineDriveDel, SIGNAL(clicked()), this, SLOT(cmdWineDriveDel_Click()));
    cmdGetWineDesktop->setIcon(CoreLib->loadIcon("data/folder.png"));
    cmdGetWineDesktopDoc->setIcon(CoreLib->loadIcon("data/folder.png"));
    cmdGetWineDesktopPic->setIcon(CoreLib->loadIcon("data/folder.png"));
    cmdGetWineDesktopMus->setIcon(CoreLib->loadIcon("data/folder.png"));
    cmdGetWineDesktopVid->setIcon(CoreLib->loadIcon("data/folder.png"));

    return;
}

void FakeDriveSettings::cmdJoystickEdit_Click(){
    QListWidgetItem *item = listJoystickAxesMappings->currentItem();
    if (!item)
        return;

    bool ok;
    QString text = QInputDialog::getText(this, tr("Joystick Axes Mappings"), tr("Joystick axes mappings might be defined as:\n\"Joystick name\"=\"axes mapping\"\n\nFor example:\n\"Logitech Logitech Dual Action\"=\"X,Y,Rz,Slider1,POV1\"\n\nSee help for details."), QLineEdit::Normal, item->text(), &ok);
    if (ok && !text.isEmpty()){
        item->setText(text);
    }
    return;
}


void FakeDriveSettings::cmdJoystickAdd_Click(){
    bool ok;
    QString text = QInputDialog::getText(this, tr("Joystick Axes Mappings"), tr("Joystick axes mappings might be defined as:\n\"Joystick name\"=\"axes mapping\"\n\nFor example:\n\"Logitech Logitech Dual Action\"=\"X,Y,Rz,Slider1,POV1\"\n\nSee help for details."), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()){
        listJoystickAxesMappings->addItem(text);
    }
    return;
}

void FakeDriveSettings::cmdJoystickDel_Click(){
    std::auto_ptr<QListWidgetItem> item (listJoystickAxesMappings->currentItem());
    if (!item.get())
        return;

    delete item.release();
    return;
}

void FakeDriveSettings::cmdWineDriveEdit_Click(){
    std::auto_ptr<DriveListWidgetItem> item (dynamic_cast<DriveListWidgetItem*>(listWineDrives->currentItem()));

    if (!item.get())
        return;

    QStringList drives;
    drives.clear();
    if (listWineDrives->count()>0){
        for (int i=0; i<listWineDrives->count(); i++){
            if (listWineDrives->item(i)!=item.get())
                drives.append(listWineDrives->item(i)->text().left(2));
        }
    }

    WineDriveDialog drevedialog(drives, item->getLetter(), item->getPath(), item->getType());
    if (drevedialog.exec()==QDialog::Accepted){
        item->setDrive(drevedialog.getLetter(), drevedialog.getPath(), drevedialog.getType());
    }
    item.release();
    return;
}

void FakeDriveSettings::cmdWineDriveAdd_Click(){
    QStringList drives;
    drives.clear();
    if (listWineDrives->count()>0){
        for (int i=0; i<listWineDrives->count(); i++){
            drives.append(listWineDrives->item(i)->text().left(2));
        }
    }

    WineDriveDialog drevedialog(drives);
    if (drevedialog.exec()==QDialog::Accepted){
        std::auto_ptr<DriveListWidgetItem> item (new DriveListWidgetItem(listWineDrives));
        item->setDrive(drevedialog.getLetter(), drevedialog.getPath(), drevedialog.getType());
        listWineDrives->addItem(item.release());
    }
    return;
}

void FakeDriveSettings::cmdWineDriveDel_Click(){
    std::auto_ptr<QListWidgetItem> item (listWineDrives->currentItem());
    if (!item.get())
        return;

    if (item->text().left(2)=="C:"){
        QMessageBox::warning(this, tr("Error"), tr("Sorry, You can't delete or modify wine C: drive.<br>But You can change it in prefix settings dialog."));
        item.release();
        return;
    }

    delete item.release();
    return;
}

void FakeDriveSettings::loadSettings(){
    QString prefixPath = db_prefix.getPath(prefixName);
    std::auto_ptr<DriveListWidgetItem> item;

    if (prefixPath.isEmpty()){
        qDebug()<<" [EE] Cant get prefix path: "<<prefixName;
        return;
    }

    Registry reg(prefixPath);

    QStringList list;

    list.clear();
    list << "\"Desktop\""<<"\"My Music\""<<"\"My Pictures\""<<"\"My Videos\""<<"\"Personal\"";
    list = reg.readKeys("user", "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", list);

    if (list.count()==5){
        desktopFolder = CoreLib->decodeRegString(list.at(0).split("\\\\").last());
        desktopDocuments = CoreLib->decodeRegString(list.at(1).split("\\\\").last());
        desktopMusic = CoreLib->decodeRegString(list.at(2).split("\\\\").last());
        desktopPictures = CoreLib->decodeRegString(list.at(3).split("\\\\").last());
        desktopVideos = CoreLib->decodeRegString(list.at(4).split("\\\\").last());
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Can't read desktop paths!"));
        this->reject();
        return;
    }


    list.clear();
    list << "\"RegisteredOrganization\"" << "\"RegisteredOwner\"";
    list = reg.readKeys("system", "Software\\Microsoft\\Windows NT\\CurrentVersion", list);
    //HKEY_CURRENT_USER\\Software\\Wine]\n\"Version

    if (list.count()>0){
        txtOrganization->setText(list.at(0));
        txtOwner->setText(list.at(1));
    }

    list.clear();
    list << "\"Version\"";
    list = reg.readKeys("user", "Software\\Wine", list);
    if (list.count()>0){
        QString version = list.at(0);
        if (version == "win7"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 7"));
        } else if (version == "winxp"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows XP"));
        } else if (version == "win2008"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 2008"));
        } else if (version == "vista"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows Vista"));
        } else if (version == "win2003"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 2003"));
        } else if (version == "win2k"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 2000"));
        } else if (version == "winme"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows ME"));
        } else if (version == "win98"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 98"));
        } else if (version == "win95"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 95"));
        } else if (version == "nt40"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows NT 4.0"));
        } else if (version == "nt351"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows NT 3.0"));
        } else if (version == "win31"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 3.1"));
        } else if (version == "win30"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 3.0"));
        } else if (version == "win20"){
            comboFakeVersion->setCurrentIndex(comboFakeVersion->findText("Windows 2.0"));
        }
    }

    list.clear();
    list << "\"ShowCrashDialog\"";
    list = reg.readKeys("user", "Software\\Wine\\WineDbg", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            cbCrashDialog->setChecked(true);
    }

    list.clear();
    list << "\"Browsers\"" << "\"Mailers\"";
    list = reg.readKeys("user", "Software\\Wine\\WineBrowser", list);

    if (list.count()>0){
        txtFakeBrowsers->setText(list.at(0));
        txtFakeMailers->setText(list.at(1));
    }

    list.clear();
    list << "\"Multisampling\"" << "\"DirectDrawRenderer\"" << "\"RenderTargetLockMode\"" << "\"OffscreenRenderingMode\"" << "\"UseGLSL\"" << "\"VideoMemorySize\"" << "\"VideoDescription\"" << "\"VideoDriver\"" << "\"SoftwareEmulation\"" << "\"PixelShaderMode\"" << "\"VertexShaderMode\""<< "\"StrictDrawOrdering\""<< "\"Nonpower2Mode\"";
    list = reg.readKeys("user", "Software\\Wine\\Direct3D", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            comboFakeD3D_Multi->setCurrentIndex(comboFakeD3D_Multi->findText(list.at(0)));

        if (!list.at(1).isEmpty())
            comboFakeD3D_Render->setCurrentIndex(comboFakeD3D_Render->findText(list.at(1)));

        if (!list.at(2).isEmpty())
            comboFakeD3D_LMode->setCurrentIndex(comboFakeD3D_LMode->findText(list.at(2)));

        if (!list.at(3).isEmpty())
            comboFakeD3D_Offscreen->setCurrentIndex(comboFakeD3D_Offscreen->findText(list.at(3)));

        if (!list.at(4).isEmpty())
            comboFakeD3D_GLSL->setCurrentIndex(comboFakeD3D_GLSL->findText(list.at(4)));

        txtFakeVideoMemory->setText(list.at(5));
        txtFakeVideoDescription->setText(list.at(6));
        txtFakeVideoDriver->setText(list.at(7));

        if (!list.at(8).isEmpty())
            comboFakeSoftwareEmulation->setCurrentIndex(comboFakeSoftwareEmulation->findText(list.at(8)));

        if (!list.at(9).isEmpty())
            comboFakePixelShaderMode->setCurrentIndex(comboFakePixelShaderMode->findText(list.at(9)));

        if (!list.at(10).isEmpty())
            comboFakeVertexShaderMode->setCurrentIndex(comboFakeVertexShaderMode->findText(list.at(10)));

        if (!list.at(11).isEmpty())
            comboFakeD3D_SDOrder->setCurrentIndex(comboFakeD3D_SDOrder->findText(list.at(11)));

        if (!list.at(12).isEmpty())
            comboFakeD3D_N2M->setCurrentIndex(comboFakeD3D_N2M->findText(list.at(12)));
    }

    list.clear();
    list << "\"DisabledExtensions\"";
    list = reg.readKeys("user", "Software\\Wine\\OpenGL", list);

    if (list.count()>0){
        txtFakeDisabledExtensions->setText(list.at(0));
    }

    list.clear();
    list << "\"MouseWarpOverride\"";
    list = reg.readKeys("user", "Software\\Wine\\DirectInput", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            comboFakeMouseWarp->setCurrentIndex(comboFakeMouseWarp->findText(list.at(0)));
    }

    list.clear();
    list << "\"MouseWarpOverride\"";
    list = reg.readExcludedKeys("user", "Software\\Wine\\DirectInput", list, 10);

    if (list.count()>0){
        listJoystickAxesMappings->insertItems (0, list);
    }

    list.clear();
    list << "\"WheelScrollLines\"";
    list = reg.readKeys("user", "Control Panel\\Desktop", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            sboxFakeInput_scroll->setValue(list.at(0).toInt());
    }

    list.clear();
    list << "\"UsePrimarySelection\"";
    list = reg.readKeys("user", "Software\\Wine\\X11 Driver", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            comboFakeInput_selection->setCurrentIndex(comboFakeInput_selection->findText(list.at(0)));
    }

    list.clear();
    list << "\"ClientSideWithRender\"" << "\"ClientSideAntiAliasWithRender\"" << "\"ClientSideAntiAliasWithCore\"" << "\"UseXRandR\"" << "\"UseXVidMode\"";
    list = reg.readKeys("user", "Software\\Wine\\X11 Driver", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            comboFakeX11_WR->setCurrentIndex(comboFakeX11_WR->findText(list.at(0)));

        if (!list.at(1).isEmpty())
            comboFakeX11_AAR->setCurrentIndex(comboFakeX11_AAR->findText(list.at(1)));

        if (!list.at(2).isEmpty())
            comboFakeX11_AAC->setCurrentIndex(comboFakeX11_AAC->findText(list.at(2)));

        if (!list.at(3).isEmpty())
            comboFakeX11_XRandr->setCurrentIndex(comboFakeX11_XRandr->findText(list.at(3)));

        if (!list.at(4).isEmpty())
            comboFakeX11_XVid->setCurrentIndex(comboFakeX11_XVid->findText(list.at(4)));
    }

    list.clear();
    list << "\"Audio\"";
    list = reg.readKeys("user", "Software\\Wine\\Drivers", list);
    if (list.count()>0){
        if (!list.at(0).isEmpty()){
            comboFakeSound_Driver->setCurrentIndex(comboFakeSound_Driver->findText(list.at(0)));
        } else {
            comboFakeSound_Driver->setCurrentIndex(comboFakeSound_Driver->findText("disabled"));
        }
    }

    list.clear();
    list << "\"AutoScanCards\"" << "\"AutoScanDevices\"" << "\"DeviceCount\"" << "\"DeviceCTLn\"" << "\"DevicePCMn\"" << "\"UseDirectHW\"";
    list = reg.readKeys("user", "Software\\Wine\\Alsa Driver", list);

    if (list.count()>0){
        if (!list.at(0).isEmpty())
            comboFakeAlsa_asCards->setCurrentIndex(comboFakeAlsa_asCards->findText(list.at(0)));

        if (!list.at(1).isEmpty())
            comboFakeAlsa_asDevices->setCurrentIndex(comboFakeAlsa_asDevices->findText(list.at(1)));

        sboxFakeAlsa_devCount->setValue(list.at(2).toInt());
        txtFakeAlsa_CTLn->setText(list.at(3));
        txtFakeAlsa_PCMn->setText(list.at(4));

        if (!list.at(5).isEmpty())
            comboFakeAlsa_DirectHW->setCurrentIndex(comboFakeAlsa_DirectHW->findText(list.at(5)));
    }

    list.clear();
    list << "\"MaxShadowSize\"";
    list = reg.readKeys("user", "Software\\Wine\\DirectSound", list);
    if (list.count()>0){
        if (!list.at(0).isEmpty())
            sboxFakeSound_shadow->setValue(list.at(0).toInt());
    }

    QDir wineDriveDir;
    wineDriveDir.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot  );

    prefixPath.append("/dosdevices/");
    if (!wineDriveDir.cd(prefixPath)){
        qDebug()<<"Cannot cd to prefix directory: "<<prefixPath;
    } else {

        QFileInfoList drivelist = wineDriveDir.entryInfoList();
        for (int i = 0; i < drivelist.size(); ++i) {
            QFileInfo fileInfo = drivelist.at(i);
            QString path = "";
            if (fileInfo.fileName().toUpper()=="C:"){
                path = "../drive_c";
            } else {
                path = fileInfo.symLinkTarget();
            }

            list.clear();
            list<<QString("\"%1\"").arg(fileInfo.fileName());
            list = reg.readKeys("system", "Software\\Wine\\Drives", list);

            item.reset(new DriveListWidgetItem(listWineDrives));
            item->setDrive(fileInfo.fileName().toUpper(), path, list.at(0));
            listWineDrives->addItem(item.release());
        }
    }

    prefixPath.append("c:/users/");
    prefixPath.append(getenv("USER"));

    if (!wineDriveDir.cd(prefixPath)){
        qDebug()<<"Cannot cd to prefix directory: "<<prefixPath;
    } else {
        QFileInfo fileinfo(QString("%1/%2").arg(prefixPath).arg(desktopFolder));
        if (fileinfo.isSymLink()){
            txtWineDesktop->setText(fileinfo.symLinkTarget());
        } else {
            txtWineDesktop->setText(fileinfo.filePath());
        }

        fileinfo.setFile(QString("%1/%2").arg(prefixPath).arg(this->desktopDocuments));
        if (fileinfo.isSymLink()){
            txtWineDesktopDoc->setText(fileinfo.symLinkTarget());
        } else {
            txtWineDesktopDoc->setText(fileinfo.filePath());
        }

        fileinfo.setFile(QString("%1/%2").arg(prefixPath).arg(this->desktopMusic));
        if (fileinfo.isSymLink()){
            txtWineDesktopMus->setText(fileinfo.symLinkTarget());
        } else {
            txtWineDesktopMus->setText(fileinfo.filePath());
        }

        fileinfo.setFile(QString("%1/%2").arg(prefixPath).arg(this->desktopPictures));
        if (fileinfo.isSymLink()){
            txtWineDesktopPic->setText(fileinfo.symLinkTarget());
        } else {
            txtWineDesktopPic->setText(fileinfo.filePath());
        }

        fileinfo.setFile(QString("%1/%2").arg(prefixPath).arg(this->desktopVideos));
        if (fileinfo.isSymLink()){
            txtWineDesktopVid->setText(fileinfo.symLinkTarget());
        } else {
            txtWineDesktopVid->setText(fileinfo.filePath());
        }
    }
}

void FakeDriveSettings::loadDefaultSettings(){

    ExecObject execObj;
    execObj.cmdargs = "-u -i";
    execObj.execcmd = "wineboot";

    if (!CoreLib->runWineBinary(execObj, prefixName, false)){
        QApplication::restoreOverrideCursor();
        reject();
        return;
    }


    txtOwner->setText(getenv("USER"));

    std::auto_ptr<DriveListWidgetItem> item;
    item.reset(new DriveListWidgetItem(listWineDrives));
    item->setDrive("C:", "../drive_c", "auto");
    listWineDrives->addItem(item.release());

    if (!db_prefix.getMountPoint(prefixName).isEmpty()){
        item.reset(new DriveListWidgetItem(listWineDrives));
        item->setDrive("D:", db_prefix.getMountPoint(prefixName), "cdrom");
        listWineDrives->addItem(item.release());
    }

    item.reset(new DriveListWidgetItem(listWineDrives));
    item->setDrive("Z:", "/", "auto");
    listWineDrives->addItem(item.release());

    item.reset(new DriveListWidgetItem(listWineDrives));
    item->setDrive("H:", QString("%1/.config/q4wine/tmp").arg(QDir::homePath()), "auto");
    listWineDrives->addItem(item.release());

    /*
    txtWineDesktop->setText(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
    txtWineDesktopDoc->setText(QDir::homePath());
    txtWineDesktopMus->setText(QDir::homePath());
    txtWineDesktopPic->setText(QDir::homePath());
    txtWineDesktopVid->setText(QDir::homePath());
    */

    QString prefixPath = db_prefix.getPath(this->prefixName);

    txtWineDesktop->setText(QString("%1/desktop-integration/Desktop").arg(prefixPath));
    txtWineDesktopDoc->setText(QString("%1/desktop-integration/").arg(prefixPath));
    txtWineDesktopMus->setText(QString("%1/desktop-integration/").arg(prefixPath));
    txtWineDesktopPic->setText(QString("%1/desktop-integration/").arg(prefixPath));
    txtWineDesktopVid->setText(QString("%1/desktop-integration/").arg(prefixPath));

}

bool FakeDriveSettings::eventFilter(QObject *obj, QEvent *event){
    /*
        User select folder dialog function
    */

    if (obj->objectName()== "FakeDriveSettings")
        return FALSE;

    if (event->type() == QEvent::MouseButtonRelease) {
        QString file;

#if QT_VERSION >= 0x040500
        QFileDialog::Options options;

        if (CoreLib->getSetting("advanced", "useNativeFileDialog", false, 1)==0)
                options = QFileDialog::DontUseNativeDialog | QFileDialog::DontResolveSymlinks;

        if (obj->objectName().right(3)=="Bin"){
            file = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(),   "All files (*)", 0, options);
        } else {
            file = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath(),  options);
        }
#else
        if (obj->objectName().right(3)=="Bin"){
            file = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(),   "All files (*)");
        } else {
            file = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath());
        }
#endif

        if (!file.isEmpty()){
            QString a;
            a.append("txt");
            a.append(obj->objectName().right(obj->objectName().length()-6));

            std::auto_ptr<QLineEdit> lineEdit (findChild<QLineEdit *>(a));
            if (lineEdit.get()){
                lineEdit->setText(file);
            } else {
                qDebug("Error");
            }
            lineEdit.release();

            if (obj==cmdGetWineDesktop){
                txtWineDesktopDoc->setText(file);
                txtWineDesktopPic->setText(file);
                txtWineDesktopMus->setText(file);
                txtWineDesktopVid->setText(file);
            }
        }
    }
    return FALSE;
}