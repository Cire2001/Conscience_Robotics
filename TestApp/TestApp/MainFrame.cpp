#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>

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

    m_snapshotButton = new wxButton(panel, wxID_ANY, "Capture Image");
    m_snapshotButton->SetBackgroundColour(wxColour(100, 100, 200)); // Bleu
    m_snapshotButton->Bind(wxEVT_BUTTON, &MainFrame::OnSnapshotButton, this);

    m_xProfileButton = new wxButton(panel, wxID_ANY, "Profil X");
    m_xProfileButton->SetBackgroundColour(wxColour(200, 100, 100)); // Rouge
    m_xProfileButton->Bind(wxEVT_BUTTON, &MainFrame::OnXProfileButton, this);

    // Ajout des boutons au sizer horizontal avec des marges
    buttonSizer->Add(m_captureButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_snapshotButton, 1, wxALL | wxEXPAND, 10);
    buttonSizer->Add(m_xProfileButton, 1, wxALL | wxEXPAND, 10);

    // Création des textes explicatifs
    m_captureText = new wxStaticText(panel, wxID_ANY, "Capture le flux en temps réel de la caméra");
    m_snapshotText = new wxStaticText(panel, wxID_ANY, "Capture une image et la sauvegarde dans un fichier zip");
    m_xProfileText = new wxStaticText(panel, wxID_ANY, "Récupère les 10 derniers posts d'un profil X");

    // Création d'un sizer pour les textes
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);

    // Ajout des textes au sizer
    textSizer->Add(m_captureText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_snapshotText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_xProfileText, 1, wxALL | wxALIGN_CENTER, 10);

    // Ajout des sizers au sizer principal
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(textSizer, 0, wxEXPAND | wxALL, 10);

    // Création d'un placeholder pour le panel vidéo (sera créé lors du clic sur le bouton)
    wxPanel* placeholderPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
    placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);

    mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);

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

        // Supprimer le placeholder ou le panel vidéo existant
        if (mainSizer->GetItemCount() > 2)
        {
            wxSizerItem* item = mainSizer->GetItem(2);
            if (item)
            {
                wxWindow* oldPanel = item->GetWindow();
                if (oldPanel)
                {
                    mainSizer->Detach(oldPanel);
                    oldPanel->Destroy();
                }
            }
        }

        // Créer un nouveau panel vidéo
        m_videoPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
        m_videoPanel->SetBackgroundColour(*wxBLACK);
        m_videoPanel->Bind(wxEVT_PAINT, &MainFrame::OnPaint, this);

        // Ajouter le panel vidéo au sizer principal
        mainSizer->Add(m_videoPanel, 1, wxEXPAND | wxALL, 10);
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
            if (mainSizer->GetItemCount() > 2)
            {
                wxSizerItem* item = mainSizer->GetItem(2);
                if (item)
                {
                    wxWindow* oldPanel = item->GetWindow();
                    if (oldPanel)
                    {
                        mainSizer->Detach(oldPanel);
                        oldPanel->Destroy();
                        m_videoPanel = nullptr;
                    }
                }
            }

            // Recréer un placeholder
            wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
            placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
            mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);
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

        // Supprimer le panel vidéo
        if (mainSizer->GetItemCount() > 2)
        {
            wxSizerItem* item = mainSizer->GetItem(2);
            if (item)
            {
                wxWindow* oldPanel = item->GetWindow();
                if (oldPanel)
                {
                    mainSizer->Detach(oldPanel);
                    oldPanel->Destroy();
                    m_videoPanel = nullptr;
                }
            }
        }

        // Recréer un placeholder
        wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
        placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
        mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);
        mainSizer->Layout();
    }
}

void MainFrame::OnSnapshotButton(wxCommandEvent& event)
{
    // Cette fonction sera implémentée plus tard
    wxMessageBox("Fonctionnalité de capture d'image à implémenter", "Info", wxICON_INFORMATION);
}

void MainFrame::OnXProfileButton(wxCommandEvent& event)
{
    // Cette fonction sera implémentée plus tard
    wxMessageBox("Fonctionnalité de récupération des posts X à implémenter", "Info", wxICON_INFORMATION);
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