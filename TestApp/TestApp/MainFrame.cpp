#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/dcbuffer.h>

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

    // Cr�ation des textes explicatifs
    m_captureText = new wxStaticText(panel, wxID_ANY, "Capture le flux en temps r�el de la cam�ra");
    m_snapshotText = new wxStaticText(panel, wxID_ANY, "Capture une image et la sauvegarde dans un fichier zip");
    m_xProfileText = new wxStaticText(panel, wxID_ANY, "R�cup�re les 10 derniers posts d'un profil X");

    // Cr�ation d'un sizer pour les textes
    wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);

    // Ajout des textes au sizer
    textSizer->Add(m_captureText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_snapshotText, 1, wxALL | wxALIGN_CENTER, 10);
    textSizer->Add(m_xProfileText, 1, wxALL | wxALIGN_CENTER, 10);

    // Ajout des sizers au sizer principal
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(textSizer, 0, wxEXPAND | wxALL, 10);

    // Cr�ation d'un placeholder pour le panel vid�o (sera cr�� lors du clic sur le bouton)
    wxPanel* placeholderPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
    placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);

    mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);

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

        // Supprimer le placeholder ou le panel vid�o existant
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

        // Cr�er un nouveau panel vid�o
        m_videoPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
        m_videoPanel->SetBackgroundColour(*wxBLACK);
        m_videoPanel->Bind(wxEVT_PAINT, &MainFrame::OnPaint, this);

        // Ajouter le panel vid�o au sizer principal
        mainSizer->Add(m_videoPanel, 1, wxEXPAND | wxALL, 10);
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

            // Recr�er un placeholder
            wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
            placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
            mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);
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

        // Supprimer le panel vid�o
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

        // Recr�er un placeholder
        wxPanel* placeholderPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxSize(640, 480));
        placeholderPanel->SetBackgroundColour(*wxLIGHT_GREY);
        mainSizer->Add(placeholderPanel, 1, wxEXPAND | wxALL, 10);
        mainSizer->Layout();
    }
}

void MainFrame::OnSnapshotButton(wxCommandEvent& event)
{
    // Cette fonction sera impl�ment�e plus tard
    wxMessageBox("Fonctionnalit� de capture d'image � impl�menter", "Info", wxICON_INFORMATION);
}

void MainFrame::OnXProfileButton(wxCommandEvent& event)
{
    // Cette fonction sera impl�ment�e plus tard
    wxMessageBox("Fonctionnalit� de r�cup�ration des posts X � impl�menter", "Info", wxICON_INFORMATION);
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