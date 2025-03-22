#include "MainFrame.h"
#include <wx/wx.h>

MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
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

    m_snapshotButton = new wxButton(panel, wxID_ANY, "Capture Image");
    m_snapshotButton->SetBackgroundColour(wxColour(100, 100, 200)); // Bleu

    m_xProfileButton = new wxButton(panel, wxID_ANY, "Profil X");
    m_xProfileButton->SetBackgroundColour(wxColour(200, 100, 100)); // Rouge

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

    // D�finition du sizer pour le panel
    panel->SetSizer(mainSizer);

    // Centrer la fen�tre
    Centre();
}