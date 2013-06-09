;;; A window tagging mechanism for nwm
;;;
;;; Copyright (C) 2013  Brandon Invergo
;;;
;;; This program is free software; you can redistribute it and/or 
;;; modify it under the terms of the GNU General Public License 
;;; as published by the Free Software Foundation; either version 2 
;;; of the License, or (at your option) any later version. 
;;;
;;; This program is distributed in the hope that it will be useful, 
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of 
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
;;; GNU General Public License for more details. 
;;;
;;; You should have received a copy of the GNU General Public License 
;;; along with this program; if not, write to the Free Software 
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
;;; 02110-1301, USA 

;;; This file defines procedures for implementing a window tagging mechanism

; Our assoc list of tags. For each member of the list, the car is a
; boolean indicating the tag's visibility state and the cdr is the
; list of clients with that tag
(define tags-assoc '())

; New hooks
(define tag-client-hook (make-hook 2))

(define untag-client-hook (make-hook 2))

; Dynamically create a new tag, which can be any type of object, and
; set its initial visibility
(define (create-tag tag visible)
  (set! tags-assoc (acons tag (cons visible '()) tags-assoc)))

; Dynamically remove a tag, assigning any orphan clients to the first
; tag
(define (destroy-tag tag)
  (if (assoc tag tags-assoc)
      (let ((clients (tag-get-clients tag)))
        (begin
          (set! tags-assoc (assoc-remove! tags-assoc tag))
          (if (not (null? clients))
              (let ((tag-rec (car tags-assoc)))
                (set! tags-assoc
                      (assoc-set! tags-assoc (car tag-rec)
                                  (cons (cadr tag-rec)
                                        (append (cddr tag-rec) clients))))))))))

; Get the clients with a given tag
(define (tag-get-clients tag)
  (if (and (not (null? tags-assoc)) (not (unspecified? tag)))
      (cdr (assoc-ref tags-assoc tag))))

; Get the visibility status of a tag
(define (tag-get-visibility tag)
  (if (and (not (null? tags-assoc)) (not (unspecified? tag)))
      (car (assoc-ref tags-assoc tag))))

; Add a client to a tag
(define (tag-add-client client tag)
  (let ((tag-vis (tag-get-visibility tag))
        (client-list (tag-get-clients tag)))
    (begin
      (set! tags-assoc (assoc-set! tags-assoc tag
                                  (cons tag-vis (append client-list (list client)))))
      (run-hook tag-client-hook client tag))))

; Remove a client from a tag
(define (tag-remove-client client tag)
  (let ((tag-vis (tag-get-visibility tag))
        (client-list (tag-get-clients tag)))
    (if (not (null? client-list))
        (if (member client client-list)
            (begin
              (set! tags-assoc (assoc-set! tags-assoc tag
                                          (cons tag-vis (delete client client-list))))
              (run-hook untag-client-hook client tag))))))

; Toggle a tag for a client
(define (tag-toggle-client client tag)
  (let ((client-list (tag-get-clients tag)))
    (if (member client client-list)
        (tag-remove-client client tag)
        (tag-add-client client tag))))

; Helper function to toggle the visibility of a list of clients given
; their tags' visibility
(define (clients-change-visibility client-list)
  (if (not (null? client-list))
      (let ((client (car client-list)))
        (begin
          (if (client-get-visibility client)
              (if (not (mapped? client))
                  (map-client client))
              (if (mapped? client)
                  (unmap-client client)))
          (clients-change-visibility (cdr client-list))))))

; Toggle the visibility status of a tag
(define (tag-toggle-visibility tag)
  (let ((tag-vis (tag-get-visibility tag))
        (client-list (tag-get-clients tag)))
    (begin
      (assoc-set! tags-assoc tag (cons (not tag-vis) client-list))
      (clients-change-visibility client-list))))

; Switch to a single tag being visible
(define (tag-switch tag tag-assoc)
  (if (not (null? tag-assoc))
      (let ((cur-tag (caar tag-assoc)))
        (begin
          (if (equal? tag cur-tag)
              (if (not (tag-get-visibility cur-tag))
                  (tag-toggle-visibility cur-tag))
              (if (tag-get-visibility cur-tag)
                  (tag-toggle-visibility cur-tag)))
          (tag-switch tag (cdr tag-assoc))))))

; Get the list of tags associated with a client
(define (client-get-tag-list client tag-assoc)
  (if (null? tag-assoc)
      '()
      (let* ((tag (caar tag-assoc))
             (tag-vis (tag-get-visibility tag))
             (client-list (tag-get-clients tag)))
        (if (member client client-list)
            (cons tag (client-get-tag-list client (cdr tag-assoc)))
            (client-get-tag-list client (cdr tag-assoc))))))

; Get the list of visible tags
(define (get-visible-tag-list tag-assoc)
  (if (null? tag-assoc)
      '()
      (let ((tag (caar tag-assoc)))
        (if (tag-get-visibility tag)
            (cons tag (get-visible-tag-list (cdr tag-assoc)))
            (get-visible-tag-list (cdr tag-assoc))))))

; Get the visibility status of a client given the visibility of each
; of its tags.  If at least one tag is visible, the client is visible.
(define (client-get-visibility client)
  (let ((client-tag-list (client-get-tag-list client tags-assoc)))
    (member #t (map tag-get-visibility client-tag-list))))

; Assign tags to a new client
(define (tag-new-client client)
  (if (not (null? tags-assoc))
      (let ((visible-tag-list (get-visible-tag-list tags-assoc)))
        (if (null? visible-tag-list)
            (let ((first-tag (caar tags-assoc)))
              (begin
                (tag-add-client client first-tag)
                (unmap-client client))))
        (map (lambda (tag)
               (tag-add-client client tag))
             visible-tag-list))))

; Helper function for assigning key bindings to a tag
(define (set-tag-key tag key)
  (bind-key 64 key (lambda ()
                     (if (assoc tag tags-assoc)
                         (tag-toggle-visibility tag))))
  (bind-key 65 key (lambda ()
                     (if (assoc tag tags-assoc)
                         (tag-toggle-client (get-focus-client) tag))))
  (bind-key 68 key (lambda ()
                     (if (assoc tag tags-assoc)
                         (tag-switch tag tags-assoc)))))
                     
; Create a bunch of tags corresponding to the numbers 1-9.
; Technically, anything can be used for the tags themselves
; (characters, strings, numbers, etc)
(create-tag #\9 #f)
(set-tag-key #\9 "9")

(create-tag #\8 #f)
(set-tag-key #\8 "8")

(create-tag #\7 #f)
(set-tag-key #\7 "7")

(create-tag #\6 #f)
(set-tag-key #\6 "6")

(create-tag #\5 #f)
(set-tag-key #\5 "5")

(create-tag #\4 #f)
(set-tag-key #\4 "4")

(create-tag #\3 #f)
(set-tag-key #\3 "3")

(create-tag #\2 #f)
(set-tag-key #\2 "2")

(create-tag #\1 #t)
(set-tag-key #\1 "1")

; When a new client is created, run the tag it appropriately.
(add-hook! create-client-hook tag-new-client)

; When a client is detroyed, remove it from any tags 
(add-hook! destroy-client-hook (lambda (client)
                                 (map (lambda (tag)
                                        (tag-remove-client client tag))
                                      (client-get-tag-list client tags-assoc))))

; When a tag is removed from a client, unmap it as necessary
(add-hook! untag-client-hook (lambda (client tag)
                               (if (not (client-get-visibility client))
                                   (if (mapped? client)
                                       (unmap-client client)))))
