#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/statbmp.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_TIMER(wxID_ANY, MainFrame::OnTimer)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
m_isCapturing(false), m_videoPanel(nullptr), m_timer(nullptr)
{
    // Cr�ation d'un panel principal
    wxPanel* panel = new wxPanel(this);

    // Cr�ation d'un sizer vertical pour organiser les �l�ments
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Cr�ation d'un sizer horizontal pour les boutons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    // Cr�ation des boutons avec diff�rentes couleurs
    m_captureButton = new wxButton(panel, wxID_ANY, "Capture Vid�o");
    m_captureButton->SetBackgroundColour(wxColour(100, 200, 100)); // Vert
    m_captureButton->Bind(wxEVT_BUTTON, &MainFrame::OnCaptureButton, this);

    m_snapshotButton = new wxButton(panel, wxID_ANY, "Prendre Snapshot");
    m_snapshotButton->SetBackgroundColour(wxColour(100, 100, 200)); // Bleu
    m_snapshotButton->Bind(wxEVT_BUTTON, &MainFrame::OnSnapshotButton, this);

    m_saveImagesButton = new wxButton(panel, wxID_ANY, "Sauvegarder Images");
    m_saveImagesButton->SetBackgroundColour(wxColour(200, 200, 100)); // Jaune
    m_saveImagesButton->Bind(wxEVT_BUTTON, &MainFrame::OnSaveImagesButton, this);
    m_saveImagesButton->Enable(false); // D�sactiv� jusqu'� ce qu'un snapshot soit pris

    m_xProfileButton = new wxButton(panel, wxID_ANY, "Profil X");
    m_xProfileButton->SetBackgroundColour(wxColour(200, 100, 100)); // Rouge
    m_xProfileButton->Bind(wxEVT_BUTTON, &MainFrame::OnXProfileButton, this);

    // Ajout des boutons au sizer horizontal avec des marges
    buttonSizer->Add(m_captureButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_snapshotButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_saveImagesButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_xProfileButton, 1, wxALL | wxEXPAND, 10);

    // Cr�ation des textes explicatifs
    m_captureText = new wxStaticText(panel, wxID_ANY, "Capture le flux en temps r�el de la cam�ra");
    m_snapshotText = new wxStaticText(panel, wxID_ANY, "Prend un snapshot du flux vid�o");
    wxStaticText* saveImagesText = new wxStaticText(panel, wxID_ANY, "Sauvegarde tous les snapshots");
    m_xProfileText = new wxStaticText(panel, wxID_ANY, "R�cup�re les 10 derniers posts d'un profil X");

    // Cr�ation d'un sizer pour les textes
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);

    // Ajout des textes au sizer
    textSizer->Add(m_captureText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_snapshotText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(saveImagesText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_xProfileText, 1, wxALL | wxALIGN_CENTER, 10);

    // Cr�ation d'un sizer horizontal pour le panel vid�o et la liste des snapshots
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

    // Cr�ation d'un placeholder pour le panel vid�o (sera cr�� lors du clic sur le bouton)
    wxPanel* placeholderPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
    placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);

    // Cr�ation de la liste des snapshots
    m_snapshotList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(200, 360),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    m_snapshotList->InsertColumn(0, "Snapshots", wxLIST_FORMAT_LEFT, 180);
    m_snapshotList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::OnSnapshotListItemActivated, this);

    // Ajout des �l�ments au sizer de contenu
    contentSizer->Add(placeholderPanel, 3, wxEXPAND | wxALL, 10);
    contentSizer->Add(m_snapshotList, 1, wxEXPAND | wxALL, 10);

    // Ajout des sizers au sizer principal
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(textSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 10);

    // D�finition du sizer pour le panel
    panel->SetSizer(mainSizer);

    // Initialisation du timer pour la capture vid�o (sera d�marr� lors du clic sur le bouton)
    m_timer = new wxTimer(this);

    // Centrer la fen�tre
    Centre();

    // Activer le double buffering pour �viter le scintillement
    SetDoubleBuffered(true);
}

MainFrame::~MainFrame()
{
    if (m_timer)
    {
        m_timer->Stop();
        delete m_timer;
    }

    if (m_capture.isOpened())
    {
        m_capture.release();
    }
}

void MainFrame::OnTimer(wxTimerEvent& event)
{
    if (m_capture.isOpened() && m_videoPanel)
    {
        cv::Mat frame;
        m_capture >> frame;

        if (!frame.empty())
        {
            wxMutexLocker lock(m_frameMutex);
            m_currentFrame = frame.clone();

            // Utiliser CallAfter pour �viter les probl�mes de thread
            wxTheApp->CallAfter([this]() {
                if (m_videoPanel)
                    m_videoPanel->Refresh();
                });
        }
    }
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
    wxWindow* window = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!window || window != m_videoPanel)
    {
        event.Skip();
        return;
    }

    wxBufferedPaintDC dc(m_videoPanel);

    // Effacer le fond
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();

    wxMutexLocker lock(m_frameMutex);
    if (!m_currentFrame.empty())
    {
        // Convertir l'image OpenCV en wxImage
        wxImage wxImg = ConvertOpenCVToWxImage(m_currentFrame);

        // Redimensionner l'image pour qu'elle s'adapte au panel
        wxSize panelSize = m_videoPanel->GetSize();
        wxImg = wxImg.Scale(panelSize.GetWidth(), panelSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

        // Cr�er un bitmap � partir de l'image
        wxBitmap bitmap(wxImg);

        // Dessiner le bitmap
        dc.DrawBitmap(bitmap, 0, 0);
    }

    event.Skip();
}

void MainFrame::OnCaptureButton(wxCommandEvent& event)
{
    if (!m_isCapturing)
    {
        // R�cup�rer le panel principal
        wxPanel* mainPanel = dynamic_cast<wxPanel*>(GetChildren()[0]);
        if (!mainPanel) return;

        wxSizer* mainSizer = mainPanel->GetSizer();
        if (!mainSizer) return;

        // R�cup�rer le sizer de contenu (3�me �l�ment)
        wxSizerItem* contentSizerItem = mainSizer->GetItem((size_t)2);
        if (!contentSizerItem) return;

        wxSizer* contentSizer = contentSizerItem->GetSizer();
        if (!contentSizer) return;

        // Supprimer le placeholder ou le panel vid�o existant
        if (contentSizer->GetItemCount() > 0)
        {
            wxSizerItem* item = contentSizer->GetItem((size_t)0);
            if (item)
            {
                wxWindow* oldPanel = item->GetWindow();
                if (oldPanel)
                {
                    contentSizer->Detach(oldPanel);
                    oldPanel->Destroy();
                }
            }
        }

        // Cr�er un nouveau panel vid�o
        m_videoPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
        m_videoPanel->SetBackgroundColour(*wxBLACK);
        m_videoPanel->Bind(wxEVT_PAINT, &MainFrame::OnPaint, this);

        // Ajouter le panel vid�o au sizer de contenu
        contentSizer->Insert(0, m_videoPanel, 3, wxEXPAND | wxALL, 10);
        contentSizer->Layout();
        mainSizer->Layout();

        // D�marrer la capture vid�o
        m_capture.open(0); // 0 pour la cam�ra par d�faut

        if (m_capture.isOpened())
        {
            m_isCapturing = true;
            m_timer->Start(33); // ~30 FPS
            m_captureButton->SetLabel("Arr�ter Capture");
        }
        else
        {
            wxMessageBox("Impossible d'ouvrir la cam�ra", "Erreur", wxICON_ERROR);

            // Supprimer le panel vid�o en cas d'erreur
            if (contentSizer->GetItemCount() > 0)
            {
                wxSizerItem* item = contentSizer->GetItem((size_t)0);
                if (item)
                {
                    wxWindow* oldPanel = item->GetWindow();
                    if (oldPanel)
                    {
                        contentSizer->Detach(oldPanel);
                        oldPanel->Destroy();
                        m_videoPanel = nullptr;
                    }
                }
            }

            // Recr�er un placeholder
            wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
            placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
            contentSizer->Insert(0, placeholderPanel, 3, wxEXPAND | wxALL, 10);
            contentSizer->Layout();
            mainSizer->Layout();
        }
    }
    else
    {
        // Arr�ter la capture vid�o
        m_timer->Stop();
        m_capture.release();
        m_isCapturing = false;
        m_captureButton->SetLabel("Capture Vid�o");

        // Effacer la frame courante
        wxMutexLocker lock(m_frameMutex);
        m_currentFrame = cv::Mat();

        // R�cup�rer le panel principal
        wxPanel* mainPanel = dynamic_cast<wxPanel*>(GetChildren()[0]);
        if (!mainPanel) return;

        wxSizer* mainSizer = mainPanel->GetSizer();
        if (!mainSizer) return;

        // R�cup�rer le sizer de contenu (3�me �l�ment)
        wxSizerItem* contentSizerItem = mainSizer->GetItem((size_t)2);
        if (!contentSizerItem) return;

        wxSizer* contentSizer = contentSizerItem->GetSizer();
        if (!contentSizer) return;

        // Supprimer le panel vid�o
        if (contentSizer->GetItemCount() > 0)
        {
            wxSizerItem* item = contentSizer->GetItem((size_t)0);
            if (item)
            {
                wxWindow* oldPanel = item->GetWindow();
                if (oldPanel)
                {
                    contentSizer->Detach(oldPanel);
                    oldPanel->Destroy();
                    m_videoPanel = nullptr;
                }
            }
        }

        // Recr�er un placeholder
        wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
        placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
        contentSizer->Insert(0, placeholderPanel, 3, wxEXPAND | wxALL, 10);
        contentSizer->Layout();
        mainSizer->Layout();
    }
}

void MainFrame::OnSnapshotButton(wxCommandEvent& event)
{
    // V�rifier si la capture vid�o est active
    if (!m_isCapturing || !m_capture.isOpened())
    {
        wxMessageBox("Veuillez d'abord d�marrer la capture vid�o", "Information", wxICON_INFORMATION);
        return;
    }

    // Capturer l'image actuelle
    cv::Mat snapshot;
    {
        wxMutexLocker lock(m_frameMutex);
        if (m_currentFrame.empty())
        {
            wxMessageBox("Impossible de capturer l'image", "Erreur", wxICON_ERROR);
            return;
        }
        snapshot = m_currentFrame.clone();
    }

    // Ajouter le snapshot � la liste
    m_snapshots.push_back(snapshot);

    // Ajouter une entr�e dans la liste des snapshots
    wxString snapshotName = wxString::Format("Snapshot %d", (int)m_snapshots.size());
    long itemIndex = m_snapshotList->InsertItem(m_snapshotList->GetItemCount(), snapshotName);
    m_snapshotList->SetItemData(itemIndex, m_snapshots.size() - 1); // Stocker l'index du snapshot

    // Activer le bouton de sauvegarde des images
    m_saveImagesButton->Enable(true);

    // Informer l'utilisateur
    wxMessageBox("Snapshot captur� avec succ�s", "Information", wxICON_INFORMATION);
}

void MainFrame::OnSaveImagesButton(wxCommandEvent& event)
{
    // V�rifier s'il y a des snapshots � sauvegarder
    if (m_snapshots.empty())
    {
        wxMessageBox("Aucun snapshot � sauvegarder", "Information", wxICON_INFORMATION);
        return;
    }

    // Afficher une bo�te de dialogue pour choisir o� sauvegarder les images
    wxDirDialog dirDialog(this, "Choisir un dossier pour sauvegarder les snapshots", "",
        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dirDialog.ShowModal() == wxID_CANCEL)
        return;

    // Sauvegarder les images dans le dossier
    wxString folderPath = dirDialog.GetPath();

    if (SaveImagesToFolder(m_snapshots, folderPath))
    {
        wxMessageBox("Snapshots sauvegard�s avec succ�s dans " + folderPath, "Succ�s", wxICON_INFORMATION);

        // Vider la liste des snapshots apr�s sauvegarde
        m_snapshots.clear();
        m_snapshotList->DeleteAllItems();
        m_saveImagesButton->Enable(false);
    }
    else
    {
        wxMessageBox("Erreur lors de la sauvegarde des snapshots", "Erreur", wxICON_ERROR);
    }
}

void MainFrame::OnXProfileButton(wxCommandEvent& event)
{
    // Cette fonction sera impl�ment�e plus tard
    wxMessageBox("Fonctionnalit� de r�cup�ration des posts X � impl�menter", "Info", wxICON_INFORMATION);
}

void MainFrame::OnSnapshotListItemActivated(wxListEvent& event)
{
    // R�cup�rer l'index du snapshot
    long itemIndex = event.GetIndex();
    long snapshotIndex = m_snapshotList->GetItemData(itemIndex);

    // V�rifier que l'index est valide
    if (snapshotIndex < 0 || snapshotIndex >= (long)m_snapshots.size())
        return;

    // Afficher le snapshot dans une nouvelle fen�tre
    cv::Mat snapshot = m_snapshots[snapshotIndex];

    // Convertir en wxImage
    wxImage image = ConvertOpenCVToWxImage(snapshot);

    // Cr�er une fen�tre pour afficher l'image
    wxFrame* frame = new wxFrame(this, wxID_ANY, "Aper�u du snapshot", wxDefaultPosition, wxSize(640, 480));

    // Cr�er un panel pour l'image
    wxPanel* panel = new wxPanel(frame);

    // Cr�er un sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Cr�er un bitmap � partir de l'image
    wxBitmap bitmap(image);

    // Cr�er un contr�le statique pour afficher l'image
    wxStaticBitmap* staticBitmap = new wxStaticBitmap(panel, wxID_ANY, bitmap);

    // Ajouter le contr�le au sizer
    sizer->Add(staticBitmap, 1, wxEXPAND | wxALL, 10);

    // D�finir le sizer pour le panel
    panel->SetSizer(sizer);

    // Ajuster la taille de la fen�tre � celle de l'image
    frame->SetClientSize(bitmap.GetWidth() + 20, bitmap.GetHeight() + 20);

    // Afficher la fen�tre
    frame->Show();
}

wxImage MainFrame::ConvertOpenCVToWxImage(const cv::Mat& image)
{
    cv::Mat rgb;

    // Convertir en RGB si n�cessaire
    if (image.channels() == 3)
    {
        cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
    }
    else if (image.channels() == 1)
    {
        cv::cvtColor(image, rgb, cv::COLOR_GRAY2RGB);
    }
    else
    {
        rgb = image.clone();
    }

    // Cr�er une wxImage � partir des donn�es OpenCV
    // IMPORTANT: Cr�er une copie des donn�es pour �viter les probl�mes de m�moire
    unsigned char* wxData = (unsigned char*)malloc(rgb.cols * rgb.rows * 3);
    memcpy(wxData, rgb.data, rgb.cols * rgb.rows * 3);

    wxImage wxImg(rgb.cols, rgb.rows, wxData);

    return wxImg;
}

bool MainFrame::SaveImagesToFolder(const std::vector<cv::Mat>& images, const wxString& folderPath)
{
    try
    {
        // Cr�er un sous-dossier avec la date et l'heure actuelles
        wxString timestamp = wxDateTime::Now().Format("%Y%m%d_%H%M%S");
        wxString snapshotFolderPath = folderPath + wxFileName::GetPathSeparator() + "snapshots_" + timestamp;

        // Cr�er le dossier
        if (!wxMkdir(snapshotFolderPath))
        {
            wxLogError("Impossible de cr�er le dossier %s", snapshotFolderPath);
            return false;
        }

        // Sauvegarder chaque image
        for (size_t i = 0; i < images.size(); ++i)
        {
            // G�n�rer un nom de fichier pour l'image
            wxString imageFileName = wxString::Format("snapshot_%03zu.jpg", i + 1);
            wxString fullPath = snapshotFolderPath + wxFileName::GetPathSeparator() + imageFileName;

            // Sauvegarder l'image en format JPEG
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(95); // Qualit� de l'image (0-100)

            if (!cv::imwrite(fullPath.ToStdString(), images[i], compression_params))
            {
                wxLogError("Impossible de sauvegarder l'image %s", fullPath);
                return false;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        wxLogError("Exception lors de la sauvegarde des images: %s", e.what());
        return false;
    }
    catch (...)
    {
        wxLogError("Exception inconnue lors de la sauvegarde des images");
        return false;
    }
}