/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018 Redha Gouicem <redha.gouicem@lip6.fr>
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioctl.h>


/* sert pour l'ioctl */
#define MAGIQUE 'N'
/* 0 -> pour changer le numéro de la version courante */
#define CHANGE_VERSION _IOWR(MAGIQUE, 0, char*)
#define RESTOR_VERSION _IOWR(MAGIQUE, 2, char*)