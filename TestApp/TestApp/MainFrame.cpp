#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/statbmp.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_TIMER(wxID_ANY, MainFrame::OnTimer)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
m_isCapturing(false), m_videoPanel(nullptr), m_timer(nullptr)
{
    // Création d'un panel principal
    wxPanel* panel = new wxPanel(this);

    // Création d'un sizer vertical pour organiser les éléments
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Création d'un sizer horizontal pour les boutons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    // Création des boutons avec différentes couleurs
    m_captureButton = new wxButton(panel, wxID_ANY, "Capture Vidéo");
    m_captureButton->SetBackgroundColour(wxColour(100, 200, 100)); // Vert
    m_captureButton->Bind(wxEVT_BUTTON, &MainFrame::OnCaptureButton, this);

    m_snapshotButton = new wxButton(panel, wxID_ANY, "Prendre Snapshot");
    m_snapshotButton->SetBackgroundColour(wxColour(100, 100, 200)); // Bleu
    m_snapshotButton->Bind(wxEVT_BUTTON, &MainFrame::OnSnapshotButton, this);

    m_saveZipButton = new wxButton(panel, wxID_ANY, "Sauvegarder ZIP");
    m_saveZipButton->SetBackgroundColour(wxColour(200, 200, 100)); // Jaune
    m_saveZipButton->Bind(wxEVT_BUTTON, &MainFrame::OnSaveZipButton, this);
    m_saveZipButton->Enable(false); // Désactivé jusqu'à ce qu'un snapshot soit pris

    m_xProfileButton = new wxButton(panel, wxID_ANY, "Profil X");
    m_xProfileButton->SetBackgroundColour(wxColour(200, 100, 100)); // Rouge
    m_xProfileButton->Bind(wxEVT_BUTTON, &MainFrame::OnXProfileButton, this);

    // Ajout des boutons au sizer horizontal avec des marges
    buttonSizer->Add(m_captureButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_snapshotButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_saveZipButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_xProfileButton, 1, wxALL | wxEXPAND, 10);

    // Création des textes explicatifs
    m_captureText = new wxStaticText(panel, wxID_ANY, "Capture le flux en temps réel de la caméra");
    m_snapshotText = new wxStaticText(panel, wxID_ANY, "Prend un snapshot du flux vidéo");
    wxStaticText* saveZipText = new wxStaticText(panel, wxID_ANY, "Sauvegarde tous les snapshots dans un ZIP");
    m_xProfileText = new wxStaticText(panel, wxID_ANY, "Récupère les 10 derniers posts d'un profil X");

    // Création d'un sizer pour les textes
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);

    // Ajout des textes au sizer
    textSizer->Add(m_captureText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_snapshotText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(saveZipText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_xProfileText, 1, wxALL | wxALIGN_CENTER, 10);

    // Création d'un sizer horizontal pour le panel vidéo et la liste des snapshots
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

    // Création d'un placeholder pour le panel vidéo (sera créé lors du clic sur le bouton)
    wxPanel* placeholderPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
    placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);

    // Création de la liste des snapshots
    m_snapshotList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(200, 360),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    m_snapshotList->InsertColumn(0, "Snapshots", wxLIST_FORMAT_LEFT, 180);
    m_snapshotList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::OnSnapshotListItemActivated, this);

    // Ajout des éléments au sizer de contenu
    contentSizer->Add(placeholderPanel, 3, wxEXPAND | wxALL, 10);
    contentSizer->Add(m_snapshotList, 1, wxEXPAND | wxALL, 10);

    // Ajout des sizers au sizer principal
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(textSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 10);

    // Définition du sizer pour le panel
    panel->SetSizer(mainSizer);

    // Initialisation du timer pour la capture vidéo (sera démarré lors du clic sur le bouton)
    m_timer = new wxTimer(this);

    // Centrer la fenêtre
    Centre();

    // Activer le double buffering pour éviter le scintillement
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

            // Utiliser CallAfter pour éviter les problèmes de thread
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

        // Créer un bitmap à partir de l'image
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
        // Récupérer le panel principal
        wxPanel* mainPanel = dynamic_cast<wxPanel*>(GetChildren()[0]);
        if (!mainPanel) return;

        wxSizer* mainSizer = mainPanel->GetSizer();
        if (!mainSizer) return;

        // Récupérer le sizer de contenu (3ème élément)
        wxSizerItem* contentSizerItem = mainSizer->GetItem((size_t)2);
        if (!contentSizerItem) return;

        wxSizer* contentSizer = contentSizerItem->GetSizer();
        if (!contentSizer) return;

        // Supprimer le placeholder ou le panel vidéo existant
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

        // Créer un nouveau panel vidéo
        m_videoPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
        m_videoPanel->SetBackgroundColour(*wxBLACK);
        m_videoPanel->Bind(wxEVT_PAINT, &MainFrame::OnPaint, this);

        // Ajouter le panel vidéo au sizer de contenu
        contentSizer->Insert(0, m_videoPanel, 3, wxEXPAND | wxALL, 10);
        contentSizer->Layout();
        mainSizer->Layout();

        // Démarrer la capture vidéo
        m_capture.open(0); // 0 pour la caméra par défaut

        if (m_capture.isOpened())
        {
            m_isCapturing = true;
            m_timer->Start(33); // ~30 FPS
            m_captureButton->SetLabel("Arrêter Capture");
        }
        else
        {
            wxMessageBox("Impossible d'ouvrir la caméra", "Erreur", wxICON_ERROR);

            // Supprimer le panel vidéo en cas d'erreur
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

            // Recréer un placeholder
            wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
            placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
            contentSizer->Insert(0, placeholderPanel, 3, wxEXPAND | wxALL, 10);
            contentSizer->Layout();
            mainSizer->Layout();
        }
    }
    else
    {
        // Arrêter la capture vidéo
        m_timer->Stop();
        m_capture.release();
        m_isCapturing = false;
        m_captureButton->SetLabel("Capture Vidéo");

        // Effacer la frame courante
        wxMutexLocker lock(m_frameMutex);
        m_currentFrame = cv::Mat();

        // Récupérer le panel principal
        wxPanel* mainPanel = dynamic_cast<wxPanel*>(GetChildren()[0]);
        if (!mainPanel) return;

        wxSizer* mainSizer = mainPanel->GetSizer();
        if (!mainSizer) return;

        // Récupérer le sizer de contenu (3ème élément)
        wxSizerItem* contentSizerItem = mainSizer->GetItem((size_t)2);
        if (!contentSizerItem) return;

        wxSizer* contentSizer = contentSizerItem->GetSizer();
        if (!contentSizer) return;

        // Supprimer le panel vidéo
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

        // Recréer un placeholder
        wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(480, 360));
        placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
        contentSizer->Insert(0, placeholderPanel, 3, wxEXPAND | wxALL, 10);
        contentSizer->Layout();
        mainSizer->Layout();
    }
}

void MainFrame::OnSnapshotButton(wxCommandEvent& event)
{
    // Vérifier si la capture vidéo est active
    if (!m_isCapturing || !m_capture.isOpened())
    {
        wxMessageBox("Veuillez d'abord démarrer la capture vidéo", "Information", wxICON_INFORMATION);
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

    // Ajouter le snapshot à la liste
    m_snapshots.push_back(snapshot);

    // Ajouter une entrée dans la liste des snapshots
    wxString snapshotName = wxString::Format("Snapshot %d", (int)m_snapshots.size());
    long itemIndex = m_snapshotList->InsertItem(m_snapshotList->GetItemCount(), snapshotName);
    m_snapshotList->SetItemData(itemIndex, m_snapshots.size() - 1); // Stocker l'index du snapshot

    // Activer le bouton de sauvegarde ZIP
    m_saveZipButton->Enable(true);

    // Informer l'utilisateur
    wxMessageBox("Snapshot capturé avec succès", "Information", wxICON_INFORMATION);
}

void MainFrame::OnSaveZipButton(wxCommandEvent& event)
{
    // Vérifier s'il y a des snapshots à sauvegarder
    if (m_snapshots.empty())
    {
        wxMessageBox("Aucun snapshot à sauvegarder", "Information", wxICON_INFORMATION);
        return;
    }

    // Afficher une boîte de dialogue pour choisir où sauvegarder le fichier zip
    wxFileDialog saveFileDialog(this, "Sauvegarder les snapshots dans un fichier zip", "", "snapshots.zip",
        "Fichiers ZIP (*.zip)|*.zip", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    // Sauvegarder les images dans un fichier zip
    wxString zipPath = saveFileDialog.GetPath();

    if (SaveImagesToZip(m_snapshots, zipPath))
    {
        wxMessageBox("Snapshots sauvegardés avec succès dans " + zipPath, "Succès", wxICON_INFORMATION);

        // Vider la liste des snapshots après sauvegarde
        m_snapshots.clear();
        m_snapshotList->DeleteAllItems();
        m_saveZipButton->Enable(false);
    }
    else
    {
        wxMessageBox("Erreur lors de la sauvegarde des snapshots", "Erreur", wxICON_ERROR);
    }
}

void MainFrame::OnXProfileButton(wxCommandEvent& event)
{
    // Cette fonction sera implémentée plus tard
    wxMessageBox("Fonctionnalité de récupération des posts X à implémenter", "Info", wxICON_INFORMATION);
}

void MainFrame::OnSnapshotListItemActivated(wxListEvent& event)
{
    // Récupérer l'index du snapshot
    long itemIndex = event.GetIndex();
    long snapshotIndex = m_snapshotList->GetItemData(itemIndex);

    // Vérifier que l'index est valide
    if (snapshotIndex < 0 || snapshotIndex >= (long)m_snapshots.size())
        return;

    // Afficher le snapshot dans une nouvelle fenêtre
    cv::Mat snapshot = m_snapshots[snapshotIndex];

    // Convertir en wxImage
    wxImage image = ConvertOpenCVToWxImage(snapshot);

    // Créer une fenêtre pour afficher l'image
    wxFrame* frame = new wxFrame(this, wxID_ANY, "Aperçu du snapshot", wxDefaultPosition, wxSize(640, 480));

    // Créer un panel pour l'image
    wxPanel* panel = new wxPanel(frame);

    // Créer un sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Créer un bitmap à partir de l'image
    wxBitmap bitmap(image);

    // Créer un contrôle statique pour afficher l'image
    wxStaticBitmap* staticBitmap = new wxStaticBitmap(panel, wxID_ANY, bitmap);

    // Ajouter le contrôle au sizer
    sizer->Add(staticBitmap, 1, wxEXPAND | wxALL, 10);

    // Définir le sizer pour le panel
    panel->SetSizer(sizer);

    // Ajuster la taille de la fenêtre à celle de l'image
    frame->SetClientSize(bitmap.GetWidth() + 20, bitmap.GetHeight() + 20);

    // Afficher la fenêtre
    frame->Show();
}

wxImage MainFrame::ConvertOpenCVToWxImage(const cv::Mat& image)
{
    cv::Mat rgb;

    // Convertir en RGB si nécessaire
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

    // Créer une wxImage à partir des données OpenCV
    // IMPORTANT: Créer une copie des données pour éviter les problèmes de mémoire
    unsigned char* wxData = (unsigned char*)malloc(rgb.cols * rgb.rows * 3);
    memcpy(wxData, rgb.data, rgb.cols * rgb.rows * 3);

    wxImage wxImg(rgb.cols, rgb.rows, wxData);

    return wxImg;
}

bool MainFrame::SaveImagesToZip(const std::vector<cv::Mat>& images, const wxString& zipPath)
{
    try
    {
        // Créer le fichier zip
        wxFFileOutputStream fileOut(zipPath);
        if (!fileOut.IsOk())
        {
            wxLogError("Impossible de créer le fichier ZIP %s", zipPath);
            return false;
        }

        wxZipOutputStream zipOut(fileOut);
        if (!zipOut.IsOk())
        {
            wxLogError("Erreur lors de l'initialisation du flux ZIP");
            return false;
        }

        // Ajouter chaque image au zip
        for (size_t i = 0; i < images.size(); ++i)
        {
            // Générer un nom de fichier temporaire pour l'image
            wxString tempDir = wxStandardPaths::Get().GetTempDir();
            wxString tempFileName = wxFileName::CreateTempFileName(tempDir);

            // Ajouter l'extension .jpg
            wxString imageFileName = tempFileName + ".jpg";

            // Sauvegarder l'image en format JPEG
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(95); // Qualité de l'image (0-100)

            cv::imwrite(imageFileName.ToStdString(), images[i], compression_params);

            // Ajouter l'image au zip
            wxDateTime now = wxDateTime::Now();
            wxString entryName = wxString::Format("snapshot_%03zu_%s.jpg", i + 1,
                now.Format("%Y%m%d_%H%M%S"));

            zipOut.PutNextEntry(entryName);

            // Lire le fichier image et l'écrire dans le zip
            wxFFileInputStream fileIn(imageFileName);
            if (!fileIn.IsOk())
            {
                wxLogError("Impossible d'ouvrir le fichier temporaire %s", imageFileName);
                wxRemoveFile(imageFileName);
                continue;
            }

            zipOut.Write(fileIn);

            // Fermer l'entrée
            zipOut.CloseEntry();

            // Supprimer le fichier temporaire
            wxRemoveFile(imageFileName);
        }

        // Fermer le zip
        zipOut.Close();
        fileOut.Close();

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